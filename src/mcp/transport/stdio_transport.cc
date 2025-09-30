#include "stdio_transport.h"
#include <iostream>
#include <sstream>
#include <cstdio>

namespace pbrlab {
namespace mcp {
namespace transport {

StdioTransport::StdioTransport() {}

StdioTransport::~StdioTransport() {
  stop();
}

void StdioTransport::start() {
  if (running_.exchange(true)) {
    return;  // Already running
  }

  read_thread_ = std::thread(&StdioTransport::readLoop, this);
}

void StdioTransport::stop() {
  if (!running_.exchange(false)) {
    return;  // Not running
  }

  if (read_thread_.joinable()) {
    read_thread_.join();
  }
}

void StdioTransport::send(const std::string& message) {
  writeMessage(message);
}

void StdioTransport::send(const json::JsonRpcRequest& request) {
  writeMessage(request.stringify());
}

void StdioTransport::send(const json::JsonRpcResponse& response) {
  writeMessage(response.stringify());
}

void StdioTransport::send(const json::JsonRpcNotification& notification) {
  writeMessage(notification.stringify());
}

void StdioTransport::readLoop() {
  while (running_.load()) {
    try {
      std::string message = readMessage();
      if (!message.empty() && message_handler_) {
        message_handler_(message);
      }
    } catch (const std::exception& e) {
      if (error_handler_) {
        error_handler_(std::string("Read error: ") + e.what());
      }
    }
  }
}

std::string StdioTransport::readMessage() {
  // Read headers
  std::string headers;
  std::string line;

  while (std::getline(std::cin, line)) {
    if (line == "\r" || line.empty()) {
      break;  // End of headers
    }
    headers += line + "\n";
  }

  // Parse Content-Length header
  size_t content_length = 0;
  std::istringstream header_stream(headers);
  while (std::getline(header_stream, line)) {
    if (line.find("Content-Length:") == 0) {
      std::string length_str = line.substr(15);
      // Remove any whitespace and CR
      size_t start = length_str.find_first_not_of(" \t\r\n");
      size_t end = length_str.find_last_not_of(" \t\r\n");
      if (start != std::string::npos && end != std::string::npos) {
        length_str = length_str.substr(start, end - start + 1);
        content_length = std::stoull(length_str);
      }
    }
  }

  if (content_length == 0) {
    return "";  // No content to read
  }

  // Read content
  std::string content;
  content.resize(content_length);
  std::cin.read(&content[0], content_length);

  if (!std::cin) {
    throw std::runtime_error("Failed to read message content");
  }

  return content;
}

void StdioTransport::writeMessage(const std::string& message) {
  std::lock_guard<std::mutex> lock(write_mutex_);

  // Write headers
  std::cout << "Content-Length: " << message.length() << "\r\n";
  std::cout << "\r\n";

  // Write content
  std::cout << message;
  std::cout.flush();
}

} // namespace transport
} // namespace mcp
} // namespace pbrlab