#ifndef PBRLAB_MCP_PROTOCOL_H_
#define PBRLAB_MCP_PROTOCOL_H_

#include "../json/json.h"
#include "../json/json_rpc.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace pbrlab {
namespace mcp {

// MCP Protocol version
constexpr const char* MCP_VERSION = "1.0.0";

// MCP Capabilities
struct McpCapabilities {
  struct ToolCapabilities {
    bool supported = false;
  };

  struct PromptCapabilities {
    bool supported = false;
  };

  struct ResourceCapabilities {
    bool supported = false;
    bool includeSchemas = false;
  };

  struct LoggingCapabilities {
    bool supported = false;
  };

  ToolCapabilities tools;
  PromptCapabilities prompts;
  ResourceCapabilities resources;
  LoggingCapabilities logging;

  json::JsonValue toJson() const;
  static McpCapabilities fromJson(const json::JsonValue& json);
};

// MCP Tool Definition
struct McpTool {
  std::string name;
  std::string description;
  json::JsonValue inputSchema;  // JSON Schema for tool parameters

  json::JsonValue toJson() const;
  static McpTool fromJson(const json::JsonValue& json);
};

// MCP Resource Definition
struct McpResource {
  std::string uri;
  std::string name;
  std::string description;
  std::string mimeType;

  json::JsonValue toJson() const;
  static McpResource fromJson(const json::JsonValue& json);
};

// MCP Prompt Definition
struct McpPrompt {
  std::string name;
  std::string description;
  std::vector<std::string> arguments;

  json::JsonValue toJson() const;
  static McpPrompt fromJson(const json::JsonValue& json);
};

// Tool execution result
struct ToolResult {
  bool success;
  json::JsonValue content;
  bool isError = false;

  json::JsonValue toJson() const;
  static ToolResult fromJson(const json::JsonValue& json);
};

// MCP Message types
enum class McpMessageType {
  Initialize,
  Initialized,
  ToolsList,
  ToolsCall,
  ResourcesList,
  ResourcesRead,
  PromptsList,
  PromptsGet,
  LoggingSetLevel,
  Ping,
  Progress,
  Cancelled,
  Unknown
};

class McpMessage {
public:
  static json::JsonRpcRequest createInitialize(const std::string& clientName,
                                               const std::string& clientVersion,
                                               const McpCapabilities& capabilities);

  static json::JsonRpcResponse createInitialized(const std::string& serverName,
                                                 const std::string& serverVersion,
                                                 const McpCapabilities& capabilities,
                                                 const json::JsonValue& id);

  static json::JsonRpcRequest createToolsList();
  static json::JsonRpcResponse createToolsListResponse(const std::vector<McpTool>& tools,
                                                       const json::JsonValue& id);

  static json::JsonRpcRequest createToolCall(const std::string& name,
                                             const json::JsonValue& arguments);
  static json::JsonRpcResponse createToolCallResponse(const ToolResult& result,
                                                      const json::JsonValue& id);

  static json::JsonRpcRequest createResourcesList();
  static json::JsonRpcResponse createResourcesListResponse(const std::vector<McpResource>& resources,
                                                           const json::JsonValue& id);

  static json::JsonRpcRequest createResourceRead(const std::string& uri);
  static json::JsonRpcResponse createResourceReadResponse(const std::string& content,
                                                          const std::string& mimeType,
                                                          const json::JsonValue& id);

  static json::JsonRpcRequest createPromptsList();
  static json::JsonRpcResponse createPromptsListResponse(const std::vector<McpPrompt>& prompts,
                                                         const json::JsonValue& id);

  static json::JsonRpcRequest createPromptGet(const std::string& name,
                                              const json::JsonValue& arguments);
  static json::JsonRpcResponse createPromptGetResponse(const std::string& prompt,
                                                       const json::JsonValue& id);

  static json::JsonRpcNotification createProgress(const std::string& progressToken,
                                                  double progress,
                                                  const std::string& message = "");

  static json::JsonRpcNotification createCancelled(const std::string& reason = "");

  static json::JsonRpcRequest createPing();
  static json::JsonRpcResponse createPong(const json::JsonValue& id);

  static McpMessageType getMessageType(const json::JsonRpcRequest& request);
  static McpMessageType getNotificationType(const json::JsonRpcNotification& notification);
};

} // namespace mcp
} // namespace pbrlab

#endif // PBRLAB_MCP_PROTOCOL_H_