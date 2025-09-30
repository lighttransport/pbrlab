#include "http_server.h"
#include <sstream>
#include <cstring>
#include <algorithm>
#include <iostream>
#include <chrono>
#include <thread>

#ifdef _WIN32
  #define CLOSE_SOCKET closesocket
#else
  #include <signal.h>
  #define CLOSE_SOCKET close
#endif

namespace pbrlab {
namespace mcp {
namespace transport {

std::atomic<int> HttpServer::network_init_count_{0};

bool HttpRequest::parse(const std::string& raw) {
  std::istringstream stream(raw);
  std::string line;

  // Parse request line
  if (!std::getline(stream, line)) return false;

  std::istringstream request_line(line);
  request_line >> method >> path >> version;

  // Parse headers
  while (std::getline(stream, line) && line != "\r" && !line.empty()) {
    size_t colon = line.find(':');
    if (colon != std::string::npos) {
      std::string key = line.substr(0, colon);
      std::string value = line.substr(colon + 1);
      // Trim whitespace
      value.erase(0, value.find_first_not_of(" \t\r\n"));
      value.erase(value.find_last_not_of(" \t\r\n") + 1);
      headers[key] = value;
    }
  }

  // Parse body
  std::string remaining;
  std::string body_line;
  while (std::getline(stream, body_line)) {
    remaining += body_line + "\n";
  }
  if (!remaining.empty() && remaining.back() == '\n') {
    remaining.pop_back();
  }
  body = remaining;

  return true;
}

std::string HttpRequest::getHeader(const std::string& name) const {
  auto it = headers.find(name);
  return (it != headers.end()) ? it->second : "";
}

std::string HttpResponse::toString() const {
  std::ostringstream response;
  response << "HTTP/1.1 " << status_code << " " << status_text << "\r\n";

  for (const auto& header : headers) {
    response << header.first << ": " << header.second << "\r\n";
  }

  if (!body.empty() && headers.find("Content-Length") == headers.end()) {
    response << "Content-Length: " << body.size() << "\r\n";
  }

  response << "\r\n";
  response << body;

  return response.str();
}

void HttpResponse::setContentType(const std::string& type) {
  headers["Content-Type"] = type;
}

void HttpResponse::setCORS() {
  headers["Access-Control-Allow-Origin"] = "*";
  headers["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS";
  headers["Access-Control-Allow-Headers"] = "Content-Type";
}

HttpServer::HttpServer(int port) : port_(port) {}

HttpServer::~HttpServer() {
  stop();
}

bool HttpServer::initializeNetwork() {
#ifdef _WIN32
  if (network_init_count_++ == 0) {
    WSADATA wsa_data;
    return WSAStartup(MAKEWORD(2, 2), &wsa_data) == 0;
  }
  return true;
#else
  // Ignore SIGPIPE to prevent crashes on broken connections
  signal(SIGPIPE, SIG_IGN);
  network_init_count_++;
  return true;
#endif
}

void HttpServer::cleanupNetwork() {
#ifdef _WIN32
  if (--network_init_count_ == 0) {
    WSACleanup();
  }
#else
  network_init_count_--;
#endif
}

void HttpServer::closeSocket(socket_t socket) {
  if (socket != INVALID_SOCKET_VALUE) {
    CLOSE_SOCKET(socket);
  }
}

void HttpServer::setNonBlocking(socket_t socket) {
#ifdef _WIN32
  u_long mode = 1;
  ioctlsocket(socket, FIONBIO, &mode);
#else
  int flags = fcntl(socket, F_GETFL, 0);
  fcntl(socket, F_SETFL, flags | O_NONBLOCK);
#endif
}

bool HttpServer::start() {
  if (running_.exchange(true)) {
    return false;  // Already running
  }

  if (!initializeNetwork()) {
    running_ = false;
    return false;
  }

  // Create socket
  server_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (server_socket_ == INVALID_SOCKET_VALUE) {
    cleanupNetwork();
    running_ = false;
    return false;
  }

  // Allow socket reuse
  int reuse = 1;
  setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR,
             reinterpret_cast<const char*>(&reuse), sizeof(reuse));

  // Bind socket
  sockaddr_in server_addr{};
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port_);

  if (bind(server_socket_, reinterpret_cast<sockaddr*>(&server_addr),
           sizeof(server_addr)) != 0) {
    closeSocket(server_socket_);
    cleanupNetwork();
    running_ = false;
    return false;
  }

  // Listen
  if (listen(server_socket_, SOMAXCONN) != 0) {
    closeSocket(server_socket_);
    cleanupNetwork();
    running_ = false;
    return false;
  }

  // Start accept thread
  accept_thread_ = std::thread(&HttpServer::acceptLoop, this);

  return true;
}

void HttpServer::stop() {
  if (!running_.exchange(false)) {
    return;  // Not running
  }

  // Close all active connections
  {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    for (auto& conn : active_connections_) {
      conn->connected = false;
    }
    active_connections_.clear();
  }

  closeSocket(server_socket_);
  server_socket_ = INVALID_SOCKET_VALUE;

  if (accept_thread_.joinable()) {
    accept_thread_.join();
  }

  cleanupNetwork();
}

void HttpServer::acceptLoop() {
  while (running_.load()) {
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);

    socket_t client_socket = accept(server_socket_,
                                    reinterpret_cast<sockaddr*>(&client_addr),
                                    &client_len);

    if (client_socket == INVALID_SOCKET_VALUE) {
      continue;
    }

    // Handle client in a new thread
    std::thread(&HttpServer::handleClient, this, client_socket).detach();
  }
}

void HttpServer::handleClient(socket_t client_socket) {
  try {
    HttpRequest request = readRequest(client_socket);

    // Check for SSE routes
    for (const auto& route : sse_routes_) {
      if (request.path == route.first) {
        handleSSEConnection(client_socket, route.first);
        return;
      }
    }

    // Handle regular routes
    HttpResponse response;
    response.status_code = 404;
    response.status_text = "Not Found";

    for (const auto& route : routes_) {
      if (request.path == route.first) {
        response = route.second(request);
        break;
      }
    }

    sendResponse(client_socket, response);
  } catch (const std::exception& e) {
    std::cerr << "Error handling client: " << e.what() << std::endl;
  }

  closeSocket(client_socket);
}

HttpRequest HttpServer::readRequest(socket_t socket) {
  std::string buffer;
  char temp[4096];
  bool headers_complete = false;
  size_t content_length = 0;

  // Read headers
  while (!headers_complete) {
    int bytes = recv(socket, temp, sizeof(temp) - 1, 0);
    if (bytes <= 0) break;

    temp[bytes] = '\0';
    buffer += temp;

    size_t header_end = buffer.find("\r\n\r\n");
    if (header_end != std::string::npos) {
      headers_complete = true;

      // Parse Content-Length
      size_t cl_pos = buffer.find("Content-Length:");
      if (cl_pos != std::string::npos) {
        size_t cl_end = buffer.find("\r\n", cl_pos);
        std::string cl_str = buffer.substr(cl_pos + 15, cl_end - cl_pos - 15);
        content_length = std::stoull(cl_str);
      }

      // Read remaining body if needed
      size_t body_start = header_end + 4;
      size_t body_received = buffer.size() - body_start;

      while (body_received < content_length) {
        bytes = recv(socket, temp, sizeof(temp) - 1, 0);
        if (bytes <= 0) break;
        temp[bytes] = '\0';
        buffer += temp;
        body_received += bytes;
      }
    }
  }

  HttpRequest request;
  request.parse(buffer);
  return request;
}

void HttpServer::sendResponse(socket_t socket, const HttpResponse& response) {
  std::string response_str = response.toString();
  send(socket, response_str.c_str(), response_str.size(), 0);
}

void HttpServer::handleSSEConnection(socket_t socket, const std::string& path) {
  // Send SSE headers
  std::string headers = "HTTP/1.1 200 OK\r\n";
  headers += "Content-Type: text/event-stream\r\n";
  headers += "Cache-Control: no-cache\r\n";
  headers += "Connection: keep-alive\r\n";
  headers += "Access-Control-Allow-Origin: *\r\n";
  headers += "\r\n";

  send(socket, headers.c_str(), headers.size(), 0);

  // Create SSE connection
  auto conn = std::make_shared<SSEConnectionImpl>(socket);

  {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    active_connections_.push_back(conn);
  }

  // Call handler
  auto handler_it = sse_routes_.find(path);
  if (handler_it != sse_routes_.end()) {
    handler_it->second(conn);
  }

  // Wait for connection to close
  while (conn->connected.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  // Remove from active connections
  {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    active_connections_.erase(
        std::remove(active_connections_.begin(), active_connections_.end(), conn),
        active_connections_.end());
  }
}

void HttpServer::route(const std::string& path, RequestHandler handler) {
  routes_[path] = handler;
}

void HttpServer::routeSSE(const std::string& path,
                         std::function<void(SSEConnection)> handler) {
  sse_routes_[path] = handler;
}

void HttpServer::sendSSEEvent(SSEConnection conn,
                              const std::string& event,
                              const std::string& data,
                              const std::string& id) {
  if (!conn || !conn->connected.load()) return;

  std::string message;
  if (!id.empty()) {
    message += "id: " + id + "\n";
  }
  if (!event.empty()) {
    message += "event: " + event + "\n";
  }

  // Split data by newlines for proper SSE formatting
  std::istringstream stream(data);
  std::string line;
  while (std::getline(stream, line)) {
    message += "data: " + line + "\n";
  }
  message += "\n";

  conn->send(message);
}

SSEConnectionImpl::~SSEConnectionImpl() {
  connected = false;
  HttpServer::closeSocket(socket);
}

bool SSEConnectionImpl::send(const std::string& data) {
  std::lock_guard<std::mutex> lock(write_mutex);

  if (!connected.load()) return false;

  int total_sent = 0;
  int remaining = data.size();

  while (remaining > 0) {
    int sent = ::send(socket, data.c_str() + total_sent, remaining, 0);
    if (sent <= 0) {
      connected = false;
      return false;
    }
    total_sent += sent;
    remaining -= sent;
  }

  return true;
}

HttpResponse HttpJsonRpcHandler::handle(const HttpRequest& request) {
  HttpResponse response;
  response.setCORS();

  if (request.method == "OPTIONS") {
    response.status_code = 204;
    return response;
  }

  if (request.method != "POST") {
    response.status_code = 405;
    response.status_text = "Method Not Allowed";
    return response;
  }

  try {
    json::JsonRpcRequest json_request = json::JsonRpcRequest::parse(request.body);
    json::JsonRpcResponse json_response = handler_(json_request);

    response.setContentType("application/json");
    response.body = json_response.stringify();
  } catch (const std::exception& e) {
    response.status_code = 400;
    response.status_text = "Bad Request";
    response.setContentType("application/json");

    json::JsonRpcResponse error = json::JsonRpcResponse::error(
        json::JsonRpcResponse::PARSE_ERROR, e.what());
    response.body = error.stringify();
  }

  return response;
}

} // namespace transport
} // namespace mcp
} // namespace pbrlab