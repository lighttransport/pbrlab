#include "mcp_protocol.h"

namespace pbrlab {
namespace mcp {

json::JsonValue McpCapabilities::toJson() const {
  json::JsonValue caps = json::JsonValue::object();

  if (tools.supported) {
    json::JsonValue toolsCaps = json::JsonValue::object();
    caps["tools"] = toolsCaps;
  }

  if (prompts.supported) {
    json::JsonValue promptsCaps = json::JsonValue::object();
    caps["prompts"] = promptsCaps;
  }

  if (resources.supported) {
    json::JsonValue resourcesCaps = json::JsonValue::object();
    resourcesCaps["includeSchemas"] = json::JsonValue(resources.includeSchemas);
    caps["resources"] = resourcesCaps;
  }

  if (logging.supported) {
    json::JsonValue loggingCaps = json::JsonValue::object();
    caps["logging"] = loggingCaps;
  }

  return caps;
}

McpCapabilities McpCapabilities::fromJson(const json::JsonValue& json) {
  McpCapabilities caps;

  if (json.has("tools")) {
    caps.tools.supported = true;
  }

  if (json.has("prompts")) {
    caps.prompts.supported = true;
  }

  if (json.has("resources")) {
    caps.resources.supported = true;
    if (json.at("resources").has("includeSchemas")) {
      caps.resources.includeSchemas = json.at("resources").at("includeSchemas").asBool();
    }
  }

  if (json.has("logging")) {
    caps.logging.supported = true;
  }

  return caps;
}

json::JsonValue McpTool::toJson() const {
  json::JsonValue tool = json::JsonValue::object();
  tool["name"] = json::JsonValue(name);
  tool["description"] = json::JsonValue(description);
  tool["inputSchema"] = inputSchema;
  return tool;
}

McpTool McpTool::fromJson(const json::JsonValue& json) {
  McpTool tool;
  tool.name = json.at("name").asString();
  tool.description = json.at("description").asString();
  tool.inputSchema = json.at("inputSchema");
  return tool;
}

json::JsonValue McpResource::toJson() const {
  json::JsonValue resource = json::JsonValue::object();
  resource["uri"] = json::JsonValue(uri);
  resource["name"] = json::JsonValue(name);
  resource["description"] = json::JsonValue(description);
  resource["mimeType"] = json::JsonValue(mimeType);
  return resource;
}

McpResource McpResource::fromJson(const json::JsonValue& json) {
  McpResource resource;
  resource.uri = json.at("uri").asString();
  resource.name = json.at("name").asString();
  resource.description = json.at("description").asString();
  resource.mimeType = json.at("mimeType").asString();
  return resource;
}

json::JsonValue McpPrompt::toJson() const {
  json::JsonValue prompt = json::JsonValue::object();
  prompt["name"] = json::JsonValue(name);
  prompt["description"] = json::JsonValue(description);

  json::JsonValue args = json::JsonValue::array();
  for (const auto& arg : arguments) {
    args.push(json::JsonValue(arg));
  }
  prompt["arguments"] = args;

  return prompt;
}

McpPrompt McpPrompt::fromJson(const json::JsonValue& json) {
  McpPrompt prompt;
  prompt.name = json.at("name").asString();
  prompt.description = json.at("description").asString();

  if (json.has("arguments")) {
    const auto& args = json.at("arguments").asArray();
    for (const auto& arg : args) {
      prompt.arguments.push_back(arg.asString());
    }
  }

  return prompt;
}

json::JsonValue ToolResult::toJson() const {
  json::JsonValue result = json::JsonValue::object();

  if (isError) {
    result["isError"] = json::JsonValue(true);
  }

  result["content"] = content;
  return result;
}

ToolResult ToolResult::fromJson(const json::JsonValue& json) {
  ToolResult result;

  if (json.has("isError")) {
    result.isError = json.at("isError").asBool();
  }

  result.content = json.at("content");
  result.success = !result.isError;

  return result;
}

json::JsonRpcRequest McpMessage::createInitialize(const std::string& clientName,
                                                   const std::string& clientVersion,
                                                   const McpCapabilities& capabilities) {
  json::JsonValue params = json::JsonValue::object();
  params["protocolVersion"] = json::JsonValue(MCP_VERSION);

  json::JsonValue clientInfo = json::JsonValue::object();
  clientInfo["name"] = json::JsonValue(clientName);
  clientInfo["version"] = json::JsonValue(clientVersion);
  params["clientInfo"] = clientInfo;

  params["capabilities"] = capabilities.toJson();

  return json::JsonRpcRequest("initialize", params, json::JsonValue(1));
}

json::JsonRpcResponse McpMessage::createInitialized(const std::string& serverName,
                                                    const std::string& serverVersion,
                                                    const McpCapabilities& capabilities,
                                                    const json::JsonValue& id) {
  json::JsonValue result = json::JsonValue::object();
  result["protocolVersion"] = json::JsonValue(MCP_VERSION);

  json::JsonValue serverInfo = json::JsonValue::object();
  serverInfo["name"] = json::JsonValue(serverName);
  serverInfo["version"] = json::JsonValue(serverVersion);
  result["serverInfo"] = serverInfo;

  result["capabilities"] = capabilities.toJson();

  return json::JsonRpcResponse::success(result, id);
}

json::JsonRpcRequest McpMessage::createToolsList() {
  return json::JsonRpcRequest("tools/list", json::JsonValue::null(), json::JsonValue(2));
}

json::JsonRpcResponse McpMessage::createToolsListResponse(const std::vector<McpTool>& tools,
                                                          const json::JsonValue& id) {
  json::JsonValue result = json::JsonValue::object();
  json::JsonValue toolsArray = json::JsonValue::array();

  for (const auto& tool : tools) {
    toolsArray.push(tool.toJson());
  }

  result["tools"] = toolsArray;
  return json::JsonRpcResponse::success(result, id);
}

json::JsonRpcRequest McpMessage::createToolCall(const std::string& name,
                                                const json::JsonValue& arguments) {
  json::JsonValue params = json::JsonValue::object();
  params["name"] = json::JsonValue(name);
  params["arguments"] = arguments;

  return json::JsonRpcRequest("tools/call", params, json::JsonValue(3));
}

json::JsonRpcResponse McpMessage::createToolCallResponse(const ToolResult& result,
                                                         const json::JsonValue& id) {
  json::JsonValue response = json::JsonValue::object();
  json::JsonValue content = json::JsonValue::array();
  content.push(result.toJson());
  response["content"] = content;

  return json::JsonRpcResponse::success(response, id);
}

json::JsonRpcRequest McpMessage::createResourcesList() {
  return json::JsonRpcRequest("resources/list", json::JsonValue::null(), json::JsonValue(4));
}

json::JsonRpcResponse McpMessage::createResourcesListResponse(const std::vector<McpResource>& resources,
                                                              const json::JsonValue& id) {
  json::JsonValue result = json::JsonValue::object();
  json::JsonValue resourcesArray = json::JsonValue::array();

  for (const auto& resource : resources) {
    resourcesArray.push(resource.toJson());
  }

  result["resources"] = resourcesArray;
  return json::JsonRpcResponse::success(result, id);
}

json::JsonRpcRequest McpMessage::createResourceRead(const std::string& uri) {
  json::JsonValue params = json::JsonValue::object();
  params["uri"] = json::JsonValue(uri);

  return json::JsonRpcRequest("resources/read", params, json::JsonValue(5));
}

json::JsonRpcResponse McpMessage::createResourceReadResponse(const std::string& content,
                                                             const std::string& mimeType,
                                                             const json::JsonValue& id) {
  json::JsonValue result = json::JsonValue::object();
  json::JsonValue contents = json::JsonValue::array();

  json::JsonValue contentItem = json::JsonValue::object();
  contentItem["uri"] = json::JsonValue("");  // URI is in the request
  contentItem["mimeType"] = json::JsonValue(mimeType);

  json::JsonValue text = json::JsonValue::object();
  text["text"] = json::JsonValue(content);
  contentItem["text"] = text;

  contents.push(contentItem);
  result["contents"] = contents;

  return json::JsonRpcResponse::success(result, id);
}

json::JsonRpcRequest McpMessage::createPromptsList() {
  return json::JsonRpcRequest("prompts/list", json::JsonValue::null(), json::JsonValue(6));
}

json::JsonRpcResponse McpMessage::createPromptsListResponse(const std::vector<McpPrompt>& prompts,
                                                            const json::JsonValue& id) {
  json::JsonValue result = json::JsonValue::object();
  json::JsonValue promptsArray = json::JsonValue::array();

  for (const auto& prompt : prompts) {
    promptsArray.push(prompt.toJson());
  }

  result["prompts"] = promptsArray;
  return json::JsonRpcResponse::success(result, id);
}

json::JsonRpcRequest McpMessage::createPromptGet(const std::string& name,
                                                 const json::JsonValue& arguments) {
  json::JsonValue params = json::JsonValue::object();
  params["name"] = json::JsonValue(name);
  params["arguments"] = arguments;

  return json::JsonRpcRequest("prompts/get", params, json::JsonValue(7));
}

json::JsonRpcResponse McpMessage::createPromptGetResponse(const std::string& prompt,
                                                          const json::JsonValue& id) {
  json::JsonValue result = json::JsonValue::object();
  json::JsonValue messages = json::JsonValue::array();

  json::JsonValue message = json::JsonValue::object();
  message["role"] = json::JsonValue("user");

  json::JsonValue content = json::JsonValue::object();
  content["type"] = json::JsonValue("text");
  content["text"] = json::JsonValue(prompt);

  message["content"] = content;
  messages.push(message);

  result["messages"] = messages;
  return json::JsonRpcResponse::success(result, id);
}

json::JsonRpcNotification McpMessage::createProgress(const std::string& progressToken,
                                                     double progress,
                                                     const std::string& message) {
  json::JsonValue params = json::JsonValue::object();
  params["progressToken"] = json::JsonValue(progressToken);
  params["progress"] = json::JsonValue(progress);

  if (!message.empty()) {
    params["message"] = json::JsonValue(message);
  }

  return json::JsonRpcNotification("notifications/progress", params);
}

json::JsonRpcNotification McpMessage::createCancelled(const std::string& reason) {
  json::JsonValue params = json::JsonValue::object();

  if (!reason.empty()) {
    params["reason"] = json::JsonValue(reason);
  }

  return json::JsonRpcNotification("notifications/cancelled", params);
}

json::JsonRpcRequest McpMessage::createPing() {
  return json::JsonRpcRequest("ping", json::JsonValue::null(), json::JsonValue(999));
}

json::JsonRpcResponse McpMessage::createPong(const json::JsonValue& id) {
  return json::JsonRpcResponse::success(json::JsonValue::object(), id);
}

McpMessageType McpMessage::getMessageType(const json::JsonRpcRequest& request) {
  const std::string& method = request.method();

  if (method == "initialize") return McpMessageType::Initialize;
  if (method == "tools/list") return McpMessageType::ToolsList;
  if (method == "tools/call") return McpMessageType::ToolsCall;
  if (method == "resources/list") return McpMessageType::ResourcesList;
  if (method == "resources/read") return McpMessageType::ResourcesRead;
  if (method == "prompts/list") return McpMessageType::PromptsList;
  if (method == "prompts/get") return McpMessageType::PromptsGet;
  if (method == "logging/setLevel") return McpMessageType::LoggingSetLevel;
  if (method == "ping") return McpMessageType::Ping;

  return McpMessageType::Unknown;
}

McpMessageType McpMessage::getNotificationType(const json::JsonRpcNotification& notification) {
  const std::string& method = notification.method();

  if (method == "notifications/progress") return McpMessageType::Progress;
  if (method == "notifications/cancelled") return McpMessageType::Cancelled;

  return McpMessageType::Unknown;
}

} // namespace mcp
} // namespace pbrlab