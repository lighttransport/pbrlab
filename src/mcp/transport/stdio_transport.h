#ifndef PBRLAB_MCP_STDIO_TRANSPORT_H_
#define PBRLAB_MCP_STDIO_TRANSPORT_H_

#include "../json/json.h"
#include "../json/json_rpc.h"
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>

namespace pbrlab {
namespace mcp {
namespace transport {

class StdioTransport {
public:
  using MessageHandler = std::function<void(const std::string& message)>;
  using ErrorHandler = std::function<void(const std::string& error)>;

  StdioTransport();
  ~StdioTransport();

  void start();
  void stop();
  bool isRunning() const { return running_.load(); }

  void send(const std::string& message);
  void send(const json::JsonRpcRequest& request);
  void send(const json::JsonRpcResponse& response);
  void send(const json::JsonRpcNotification& notification);

  void setMessageHandler(MessageHandler handler) { message_handler_ = handler; }
  void setErrorHandler(ErrorHandler handler) { error_handler_ = handler; }

private:
  void readLoop();
  std::string readMessage();
  void writeMessage(const std::string& message);

  std::atomic<bool> running_{false};
  std::thread read_thread_;

  MessageHandler message_handler_;
  ErrorHandler error_handler_;

  std::mutex write_mutex_;
};

} // namespace transport
} // namespace mcp
} // namespace pbrlab

#endif // PBRLAB_MCP_STDIO_TRANSPORT_H_