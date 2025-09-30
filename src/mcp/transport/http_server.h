#ifndef PBRLAB_MCP_HTTP_SERVER_H_
#define PBRLAB_MCP_HTTP_SERVER_H_

#include "../json/json.h"
#include "../json/json_rpc.h"
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <map>
#include <vector>
#include <memory>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  typedef SOCKET socket_t;
  #define INVALID_SOCKET_VALUE INVALID_SOCKET
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <fcntl.h>
  typedef int socket_t;
  #define INVALID_SOCKET_VALUE -1
#endif

namespace pbrlab {
namespace mcp {
namespace transport {

struct HttpRequest {
  std::string method;
  std::string path;
  std::string version;
  std::map<std::string, std::string> headers;
  std::string body;

  bool parse(const std::string& raw);
  std::string getHeader(const std::string& name) const;
};

struct HttpResponse {
  int status_code = 200;
  std::string status_text = "OK";
  std::map<std::string, std::string> headers;
  std::string body;

  std::string toString() const;
  void setContentType(const std::string& type);
  void setCORS();
};

class HttpServer {
public:
  using RequestHandler = std::function<HttpResponse(const HttpRequest&)>;
  using SSEConnection = std::shared_ptr<struct SSEConnectionImpl>;

  HttpServer(int port = 8080);
  ~HttpServer();

  bool start();
  void stop();
  bool isRunning() const { return running_.load(); }

  void route(const std::string& path, RequestHandler handler);
  void routeSSE(const std::string& path,
                std::function<void(SSEConnection)> handler);

  // Send SSE event
  static void sendSSEEvent(SSEConnection conn,
                           const std::string& event,
                           const std::string& data,
                           const std::string& id = "");

  int getPort() const { return port_; }
  std::string getAddress() const { return "http://localhost:" + std::to_string(port_); }

  // Made public for SSEConnectionImpl
  static void closeSocket(socket_t socket);

private:
  void acceptLoop();
  void handleClient(socket_t client_socket);
  HttpRequest readRequest(socket_t socket);
  void sendResponse(socket_t socket, const HttpResponse& response);
  void handleSSEConnection(socket_t socket, const std::string& path);

  static bool initializeNetwork();
  static void cleanupNetwork();
  static void setNonBlocking(socket_t socket);

  int port_;
  socket_t server_socket_ = INVALID_SOCKET_VALUE;
  std::atomic<bool> running_{false};
  std::thread accept_thread_;

  std::map<std::string, RequestHandler> routes_;
  std::map<std::string, std::function<void(SSEConnection)>> sse_routes_;

  std::mutex connections_mutex_;
  std::vector<SSEConnection> active_connections_;

  static std::atomic<int> network_init_count_;
};

struct SSEConnectionImpl {
  socket_t socket;
  std::atomic<bool> connected{true};
  std::mutex write_mutex;

  SSEConnectionImpl(socket_t s) : socket(s) {}
  ~SSEConnectionImpl();

  bool send(const std::string& data);
};

// HTTP/JSON-RPC handler
class HttpJsonRpcHandler {
public:
  using JsonRpcHandler = std::function<json::JsonRpcResponse(const json::JsonRpcRequest&)>;

  HttpJsonRpcHandler(JsonRpcHandler handler) : handler_(handler) {}

  HttpResponse handle(const HttpRequest& request);

private:
  JsonRpcHandler handler_;
};

} // namespace transport
} // namespace mcp
} // namespace pbrlab

#endif // PBRLAB_MCP_HTTP_SERVER_H_