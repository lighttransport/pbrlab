#include "mcp_server.h"
#include <iostream>
#include <chrono>

namespace pbrlab {
namespace mcp {

McpServer::McpServer(const std::string& name, const std::string& version)
    : server_name_(name), server_version_(version), port_(8080) {}

McpServer::~McpServer() {
  stop();
}

bool McpServer::start(TransportType type, int port) {
  if (running_) return false;

  transport_type_ = type;
  port_ = port;

  switch (type) {
    case TransportType::Stdio:
      stdio_transport_.reset(new transport::StdioTransport());
      stdio_transport_->setMessageHandler(
          [this](const std::string& msg) { handleMessage(msg); });
      stdio_transport_->setErrorHandler(
          [](const std::string& err) { std::cerr << "Transport error: " << err << std::endl; });
      stdio_transport_->start();
      break;

    case TransportType::Http:
      http_server_.reset(new transport::HttpServer(port));
      http_server_->route("/mcp",
          [this](const transport::HttpRequest& req) {
            transport::HttpJsonRpcHandler handler(
                [this](const json::JsonRpcRequest& request) {
                  return handleRequest(request);
                });
            return handler.handle(req);
          });
      if (!http_server_->start()) {
        return false;
      }
      break;

    case TransportType::HttpSSE:
      http_server_.reset(new transport::HttpServer(port));

      // Regular HTTP endpoint for requests
      http_server_->route("/mcp",
          [this](const transport::HttpRequest& req) {
            transport::HttpJsonRpcHandler handler(
                [this](const json::JsonRpcRequest& request) {
                  return handleRequest(request);
                });
            return handler.handle(req);
          });

      // SSE endpoint for server-initiated messages
      http_server_->routeSSE("/mcp/events",
          [this](transport::HttpServer::SSEConnection conn) {
            sse_connection_ = conn;
            // Keep connection alive
            while (conn->connected.load()) {
              std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            sse_connection_.reset();
          });

      if (!http_server_->start()) {
        return false;
      }
      break;
  }

  running_ = true;
  return true;
}

void McpServer::stop() {
  if (!running_) return;

  running_ = false;
  initialized_ = false;

  if (stdio_transport_) {
    stdio_transport_->stop();
    stdio_transport_.reset();
  }

  if (http_server_) {
    http_server_->stop();
    http_server_.reset();
  }

  sse_connection_.reset();
}

std::string McpServer::getHttpAddress() const {
  if (http_server_) {
    return http_server_->getAddress();
  }
  return "";
}

void McpServer::handleMessage(const std::string& message) {
  try {
    json::JsonRpcRequest request = json::JsonRpcRequest::parse(message);
    json::JsonRpcResponse response = handleRequest(request);

    if (!request.isNotification()) {
      if (stdio_transport_) {
        stdio_transport_->send(response);
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "Error handling message: " << e.what() << std::endl;
  }
}

json::JsonRpcResponse McpServer::handleRequest(const json::JsonRpcRequest& request) {
  McpMessageType type = McpMessage::getMessageType(request);

  switch (type) {
    case McpMessageType::Initialize:
      return handleInitialize(request);

    case McpMessageType::ToolsList:
      if (!initialized_) {
        return json::JsonRpcResponse::error(
            json::JsonRpcResponse::INVALID_REQUEST,
            "Server not initialized", json::JsonValue::null(), request.id());
      }
      return handleToolsList(request);

    case McpMessageType::ToolsCall:
      if (!initialized_) {
        return json::JsonRpcResponse::error(
            json::JsonRpcResponse::INVALID_REQUEST,
            "Server not initialized", json::JsonValue::null(), request.id());
      }
      return handleToolCall(request);

    case McpMessageType::ResourcesList:
      if (!initialized_) {
        return json::JsonRpcResponse::error(
            json::JsonRpcResponse::INVALID_REQUEST,
            "Server not initialized", json::JsonValue::null(), request.id());
      }
      return handleResourcesList(request);

    case McpMessageType::ResourcesRead:
      if (!initialized_) {
        return json::JsonRpcResponse::error(
            json::JsonRpcResponse::INVALID_REQUEST,
            "Server not initialized", json::JsonValue::null(), request.id());
      }
      return handleResourceRead(request);

    case McpMessageType::PromptsList:
      if (!initialized_) {
        return json::JsonRpcResponse::error(
            json::JsonRpcResponse::INVALID_REQUEST,
            "Server not initialized", json::JsonValue::null(), request.id());
      }
      return handlePromptsList(request);

    case McpMessageType::PromptsGet:
      if (!initialized_) {
        return json::JsonRpcResponse::error(
            json::JsonRpcResponse::INVALID_REQUEST,
            "Server not initialized", json::JsonValue::null(), request.id());
      }
      return handlePromptGet(request);

    case McpMessageType::Ping:
      return handlePing(request);

    default:
      return json::JsonRpcResponse::error(
          json::JsonRpcResponse::METHOD_NOT_FOUND,
          "Unknown method: " + request.method(),
          json::JsonValue::null(), request.id());
  }
}

json::JsonRpcResponse McpServer::handleInitialize(const json::JsonRpcRequest& request) {
  initialized_ = true;

  // Parse client capabilities if provided
  if (request.params().has("capabilities")) {
    // Could store client capabilities if needed
  }

  return McpMessage::createInitialized(server_name_, server_version_,
                                       capabilities_, request.id());
}

json::JsonRpcResponse McpServer::handleToolsList(const json::JsonRpcRequest& request) {
  std::lock_guard<std::mutex> lock(tools_mutex_);

  std::vector<McpTool> tool_list;
  for (const auto& pair : tools_) {
    tool_list.push_back(pair.second);
  }

  return McpMessage::createToolsListResponse(tool_list, request.id());
}

json::JsonRpcResponse McpServer::handleToolCall(const json::JsonRpcRequest& request) {
  if (!request.params().has("name")) {
    return json::JsonRpcResponse::error(
        json::JsonRpcResponse::INVALID_PARAMS,
        "Tool name is required", json::JsonValue::null(), request.id());
  }

  std::string tool_name = request.params().at("name").asString();
  json::JsonValue arguments = request.params().has("arguments") ?
                             request.params().at("arguments") : json::JsonValue::null();

  std::lock_guard<std::mutex> lock(tools_mutex_);

  auto handler_it = tool_handlers_.find(tool_name);
  if (handler_it == tool_handlers_.end()) {
    return json::JsonRpcResponse::error(
        json::JsonRpcResponse::METHOD_NOT_FOUND,
        "Tool not found: " + tool_name,
        json::JsonValue::null(), request.id());
  }

  try {
    ToolResult result = handler_it->second(arguments);
    return McpMessage::createToolCallResponse(result, request.id());
  } catch (const std::exception& e) {
    ToolResult error_result;
    error_result.success = false;
    error_result.isError = true;
    error_result.content = json::JsonValue(std::string("Tool execution error: ") + e.what());
    return McpMessage::createToolCallResponse(error_result, request.id());
  }
}

json::JsonRpcResponse McpServer::handleResourcesList(const json::JsonRpcRequest& request) {
  std::lock_guard<std::mutex> lock(resources_mutex_);

  std::vector<McpResource> resource_list;
  for (const auto& pair : resources_) {
    resource_list.push_back(pair.second);
  }

  return McpMessage::createResourcesListResponse(resource_list, request.id());
}

json::JsonRpcResponse McpServer::handleResourceRead(const json::JsonRpcRequest& request) {
  if (!request.params().has("uri")) {
    return json::JsonRpcResponse::error(
        json::JsonRpcResponse::INVALID_PARAMS,
        "Resource URI is required", json::JsonValue::null(), request.id());
  }

  std::string uri = request.params().at("uri").asString();

  std::lock_guard<std::mutex> lock(resources_mutex_);

  auto resource_it = resources_.find(uri);
  if (resource_it == resources_.end()) {
    return json::JsonRpcResponse::error(
        json::JsonRpcResponse::METHOD_NOT_FOUND,
        "Resource not found: " + uri,
        json::JsonValue::null(), request.id());
  }

  auto handler_it = resource_handlers_.find(uri);
  if (handler_it == resource_handlers_.end()) {
    return json::JsonRpcResponse::error(
        json::JsonRpcResponse::INTERNAL_ERROR,
        "No handler for resource: " + uri,
        json::JsonValue::null(), request.id());
  }

  try {
    std::string content = handler_it->second(uri);
    return McpMessage::createResourceReadResponse(content,
                                                  resource_it->second.mimeType,
                                                  request.id());
  } catch (const std::exception& e) {
    return json::JsonRpcResponse::error(
        json::JsonRpcResponse::INTERNAL_ERROR,
        std::string("Resource read error: ") + e.what(),
        json::JsonValue::null(), request.id());
  }
}

json::JsonRpcResponse McpServer::handlePromptsList(const json::JsonRpcRequest& request) {
  std::lock_guard<std::mutex> lock(prompts_mutex_);

  std::vector<McpPrompt> prompt_list;
  for (const auto& pair : prompts_) {
    prompt_list.push_back(pair.second);
  }

  return McpMessage::createPromptsListResponse(prompt_list, request.id());
}

json::JsonRpcResponse McpServer::handlePromptGet(const json::JsonRpcRequest& request) {
  if (!request.params().has("name")) {
    return json::JsonRpcResponse::error(
        json::JsonRpcResponse::INVALID_PARAMS,
        "Prompt name is required", json::JsonValue::null(), request.id());
  }

  std::string prompt_name = request.params().at("name").asString();
  json::JsonValue arguments = request.params().has("arguments") ?
                             request.params().at("arguments") : json::JsonValue::null();

  std::lock_guard<std::mutex> lock(prompts_mutex_);

  auto handler_it = prompt_handlers_.find(prompt_name);
  if (handler_it == prompt_handlers_.end()) {
    return json::JsonRpcResponse::error(
        json::JsonRpcResponse::METHOD_NOT_FOUND,
        "Prompt not found: " + prompt_name,
        json::JsonValue::null(), request.id());
  }

  try {
    std::string prompt_text = handler_it->second(arguments);
    return McpMessage::createPromptGetResponse(prompt_text, request.id());
  } catch (const std::exception& e) {
    return json::JsonRpcResponse::error(
        json::JsonRpcResponse::INTERNAL_ERROR,
        std::string("Prompt generation error: ") + e.what(),
        json::JsonValue::null(), request.id());
  }
}

json::JsonRpcResponse McpServer::handlePing(const json::JsonRpcRequest& request) {
  return McpMessage::createPong(request.id());
}

void McpServer::registerTool(const std::string& name,
                             const std::string& description,
                             const json::JsonValue& inputSchema,
                             ToolHandler handler) {
  std::lock_guard<std::mutex> lock(tools_mutex_);

  McpTool tool;
  tool.name = name;
  tool.description = description;
  tool.inputSchema = inputSchema;

  tools_[name] = tool;
  tool_handlers_[name] = handler;
}

void McpServer::registerResource(const std::string& uri,
                                 const std::string& name,
                                 const std::string& description,
                                 const std::string& mimeType,
                                 ResourceHandler handler) {
  std::lock_guard<std::mutex> lock(resources_mutex_);

  McpResource resource;
  resource.uri = uri;
  resource.name = name;
  resource.description = description;
  resource.mimeType = mimeType;

  resources_[uri] = resource;
  resource_handlers_[uri] = handler;
}

void McpServer::registerPrompt(const std::string& name,
                               const std::string& description,
                               const std::vector<std::string>& arguments,
                               PromptHandler handler) {
  std::lock_guard<std::mutex> lock(prompts_mutex_);

  McpPrompt prompt;
  prompt.name = name;
  prompt.description = description;
  prompt.arguments = arguments;

  prompts_[name] = prompt;
  prompt_handlers_[name] = handler;
}

void McpServer::notifyProgress(const std::string& progressToken,
                               double progress,
                               const std::string& message) {
  json::JsonRpcNotification notification =
      McpMessage::createProgress(progressToken, progress, message);
  sendNotification(notification);
}

void McpServer::notifyCancelled(const std::string& reason) {
  json::JsonRpcNotification notification = McpMessage::createCancelled(reason);
  sendNotification(notification);
}

void McpServer::sendNotification(const json::JsonRpcNotification& notification) {
  if (stdio_transport_) {
    stdio_transport_->send(notification);
  }

  if (sse_connection_ && sse_connection_->connected.load()) {
    transport::HttpServer::sendSSEEvent(sse_connection_, "notification",
                                       notification.stringify());
  }
}

// SchemaBuilder implementation
json::JsonValue SchemaBuilder::object() {
  json::JsonValue schema = json::JsonValue::object();
  schema["type"] = json::JsonValue("object");
  return schema;
}

json::JsonValue SchemaBuilder::string(const std::string& description) {
  json::JsonValue schema = json::JsonValue::object();
  schema["type"] = json::JsonValue("string");
  if (!description.empty()) {
    schema["description"] = json::JsonValue(description);
  }
  return schema;
}

json::JsonValue SchemaBuilder::number(const std::string& description) {
  json::JsonValue schema = json::JsonValue::object();
  schema["type"] = json::JsonValue("number");
  if (!description.empty()) {
    schema["description"] = json::JsonValue(description);
  }
  return schema;
}

json::JsonValue SchemaBuilder::boolean(const std::string& description) {
  json::JsonValue schema = json::JsonValue::object();
  schema["type"] = json::JsonValue("boolean");
  if (!description.empty()) {
    schema["description"] = json::JsonValue(description);
  }
  return schema;
}

json::JsonValue SchemaBuilder::array(const json::JsonValue& items,
                                     const std::string& description) {
  json::JsonValue schema = json::JsonValue::object();
  schema["type"] = json::JsonValue("array");
  schema["items"] = items;
  if (!description.empty()) {
    schema["description"] = json::JsonValue(description);
  }
  return schema;
}

json::JsonValue SchemaBuilder::objectWithProperties(
    const std::map<std::string, json::JsonValue>& properties,
    const std::vector<std::string>& required) {
  json::JsonValue schema = json::JsonValue::object();
  schema["type"] = json::JsonValue("object");

  json::JsonValue props = json::JsonValue::object();
  for (const auto& pair : properties) {
    props[pair.first] = pair.second;
  }
  schema["properties"] = props;

  if (!required.empty()) {
    json::JsonValue req = json::JsonValue::array();
    for (const auto& field : required) {
      req.push(json::JsonValue(field));
    }
    schema["required"] = req;
  }

  return schema;
}

} // namespace mcp
} // namespace pbrlab