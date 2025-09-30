#ifndef PBRLAB_MCP_SERVER_H_
#define PBRLAB_MCP_SERVER_H_

#include "mcp_protocol.h"
#include "../json/json_rpc.h"
#include "../transport/stdio_transport.h"
#include "../transport/http_server.h"
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <mutex>

namespace pbrlab {
namespace mcp {

// Forward declarations
class McpTool;
class McpResource;
class McpPrompt;

// Tool handler function type
using ToolHandler = std::function<ToolResult(const json::JsonValue& arguments)>;

// Resource handler function type
using ResourceHandler = std::function<std::string(const std::string& uri)>;

// Prompt handler function type
using PromptHandler = std::function<std::string(const json::JsonValue& arguments)>;

class McpServer {
public:
  enum class TransportType {
    Stdio,
    Http,
    HttpSSE
  };

  McpServer(const std::string& name = "pbrlab-mcp",
           const std::string& version = "1.0.0");
  ~McpServer();

  // Server control
  bool start(TransportType type, int port = 8080);
  void stop();
  bool isRunning() const { return running_; }

  // Capabilities configuration
  void enableTools() { capabilities_.tools.supported = true; }
  void enableResources(bool includeSchemas = false) {
    capabilities_.resources.supported = true;
    capabilities_.resources.includeSchemas = includeSchemas;
  }
  void enablePrompts() { capabilities_.prompts.supported = true; }
  void enableLogging() { capabilities_.logging.supported = true; }

  // Tool registration
  void registerTool(const std::string& name,
                   const std::string& description,
                   const json::JsonValue& inputSchema,
                   ToolHandler handler);

  // Resource registration
  void registerResource(const std::string& uri,
                       const std::string& name,
                       const std::string& description,
                       const std::string& mimeType,
                       ResourceHandler handler);

  // Prompt registration
  void registerPrompt(const std::string& name,
                     const std::string& description,
                     const std::vector<std::string>& arguments,
                     PromptHandler handler);

  // Progress notification
  void notifyProgress(const std::string& progressToken,
                      double progress,
                      const std::string& message = "");

  // Cancellation notification
  void notifyCancelled(const std::string& reason = "");

  // Get server info
  std::string getName() const { return server_name_; }
  std::string getVersion() const { return server_version_; }
  int getPort() const { return port_; }
  std::string getHttpAddress() const;

private:
  // Message handling
  void handleMessage(const std::string& message);
  json::JsonRpcResponse handleRequest(const json::JsonRpcRequest& request);

  // MCP protocol handlers
  json::JsonRpcResponse handleInitialize(const json::JsonRpcRequest& request);
  json::JsonRpcResponse handleToolsList(const json::JsonRpcRequest& request);
  json::JsonRpcResponse handleToolCall(const json::JsonRpcRequest& request);
  json::JsonRpcResponse handleResourcesList(const json::JsonRpcRequest& request);
  json::JsonRpcResponse handleResourceRead(const json::JsonRpcRequest& request);
  json::JsonRpcResponse handlePromptsList(const json::JsonRpcRequest& request);
  json::JsonRpcResponse handlePromptGet(const json::JsonRpcRequest& request);
  json::JsonRpcResponse handlePing(const json::JsonRpcRequest& request);

  // Send notification
  void sendNotification(const json::JsonRpcNotification& notification);

  std::string server_name_;
  std::string server_version_;
  McpCapabilities capabilities_;

  TransportType transport_type_;
  int port_;
  bool running_ = false;
  bool initialized_ = false;

  // Transport layers
  std::unique_ptr<transport::StdioTransport> stdio_transport_;
  std::unique_ptr<transport::HttpServer> http_server_;
  transport::HttpServer::SSEConnection sse_connection_;

  // Tool registry
  std::mutex tools_mutex_;
  std::map<std::string, McpTool> tools_;
  std::map<std::string, ToolHandler> tool_handlers_;

  // Resource registry
  std::mutex resources_mutex_;
  std::map<std::string, McpResource> resources_;
  std::map<std::string, ResourceHandler> resource_handlers_;

  // Prompt registry
  std::mutex prompts_mutex_;
  std::map<std::string, McpPrompt> prompts_;
  std::map<std::string, PromptHandler> prompt_handlers_;
};

// Convenience class for building tool schemas
class SchemaBuilder {
public:
  static json::JsonValue object();
  static json::JsonValue string(const std::string& description = "");
  static json::JsonValue number(const std::string& description = "");
  static json::JsonValue boolean(const std::string& description = "");
  static json::JsonValue array(const json::JsonValue& items,
                               const std::string& description = "");

  static json::JsonValue objectWithProperties(
      const std::map<std::string, json::JsonValue>& properties,
      const std::vector<std::string>& required = {});
};

} // namespace mcp
} // namespace pbrlab

#endif // PBRLAB_MCP_SERVER_H_