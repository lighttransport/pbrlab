#include "json_rpc.h"

namespace pbrlab {
namespace mcp {
namespace json {

JsonRpcRequest JsonRpcRequest::parse(const std::string& json) {
  return parse(JsonParser::parse(json));
}

JsonRpcRequest JsonRpcRequest::parse(const JsonValue& json) {
  if (!json.isObject()) {
    throw std::runtime_error("JSON-RPC request must be an object");
  }

  const auto& obj = json.asObject();

  if (!json.has("jsonrpc") || json.at("jsonrpc").asString() != "2.0") {
    throw std::runtime_error("Invalid JSON-RPC version");
  }

  if (!json.has("method") || !json.at("method").isString()) {
    throw std::runtime_error("Method is required and must be a string");
  }

  std::string method = json.at("method").asString();
  JsonValue params = json.has("params") ? json.at("params") : JsonValue::null();
  JsonValue id = json.has("id") ? json.at("id") : JsonValue::null();

  return JsonRpcRequest(method, params, id);
}

JsonValue JsonRpcRequest::toJson() const {
  JsonValue json = JsonValue::object();
  json["jsonrpc"] = JsonValue("2.0");
  json["method"] = JsonValue(method_);

  if (!params_.isNull()) {
    json["params"] = params_;
  }

  if (!id_.isNull()) {
    json["id"] = id_;
  }

  return json;
}

std::string JsonRpcRequest::stringify() const {
  return toJson().stringify();
}

JsonRpcResponse JsonRpcResponse::success(const JsonValue& result, const JsonValue& id) {
  return JsonRpcResponse(result, JsonValue::null(), id);
}

JsonRpcResponse JsonRpcResponse::error(int code, const std::string& message,
                                       const JsonValue& data, const JsonValue& id) {
  JsonValue error = JsonValue::object();
  error["code"] = JsonValue(code);
  error["message"] = JsonValue(message);

  if (!data.isNull()) {
    error["data"] = data;
  }

  return JsonRpcResponse(JsonValue::null(), error, id);
}

JsonRpcResponse JsonRpcResponse::parse(const std::string& json) {
  return parse(JsonParser::parse(json));
}

JsonRpcResponse JsonRpcResponse::parse(const JsonValue& json) {
  if (!json.isObject()) {
    throw std::runtime_error("JSON-RPC response must be an object");
  }

  if (!json.has("jsonrpc") || json.at("jsonrpc").asString() != "2.0") {
    throw std::runtime_error("Invalid JSON-RPC version");
  }

  JsonValue result = json.has("result") ? json.at("result") : JsonValue::null();
  JsonValue error = json.has("error") ? json.at("error") : JsonValue::null();
  JsonValue id = json.has("id") ? json.at("id") : JsonValue::null();

  if (result.isNull() && error.isNull()) {
    throw std::runtime_error("Response must have either result or error");
  }

  if (!result.isNull() && !error.isNull()) {
    throw std::runtime_error("Response cannot have both result and error");
  }

  return JsonRpcResponse(result, error, id);
}

JsonValue JsonRpcResponse::toJson() const {
  JsonValue json = JsonValue::object();
  json["jsonrpc"] = JsonValue("2.0");

  if (!result_.isNull()) {
    json["result"] = result_;
  }

  if (!error_.isNull()) {
    json["error"] = error_;
  }

  json["id"] = id_;
  return json;
}

std::string JsonRpcResponse::stringify() const {
  return toJson().stringify();
}

JsonValue JsonRpcNotification::toJson() const {
  JsonValue json = JsonValue::object();
  json["jsonrpc"] = JsonValue("2.0");
  json["method"] = JsonValue(method_);

  if (!params_.isNull()) {
    json["params"] = params_;
  }

  return json;
}

std::string JsonRpcNotification::stringify() const {
  return toJson().stringify();
}

JsonRpcNotification JsonRpcNotification::parse(const std::string& json) {
  return parse(JsonParser::parse(json));
}

JsonRpcNotification JsonRpcNotification::parse(const JsonValue& json) {
  if (!json.isObject()) {
    throw std::runtime_error("JSON-RPC notification must be an object");
  }

  if (!json.has("jsonrpc") || json.at("jsonrpc").asString() != "2.0") {
    throw std::runtime_error("Invalid JSON-RPC version");
  }

  if (!json.has("method") || !json.at("method").isString()) {
    throw std::runtime_error("Method is required and must be a string");
  }

  if (json.has("id")) {
    throw std::runtime_error("Notifications must not have an id");
  }

  std::string method = json.at("method").asString();
  JsonValue params = json.has("params") ? json.at("params") : JsonValue::null();

  return JsonRpcNotification(method, params);
}

void JsonRpcServer::registerMethod(const std::string& method, JsonRpcHandler handler) {
  handlers_[method] = handler;
}

void JsonRpcServer::registerAsyncMethod(const std::string& method,
                                        JsonRpcAsyncHandler handler) {
  async_handlers_[method] = handler;
}

JsonRpcResponse JsonRpcServer::handle(const JsonRpcRequest& request) {
  const std::string& method = request.method();

  // Check sync handlers first
  auto it = handlers_.find(method);
  if (it != handlers_.end()) {
    try {
      JsonValue result = it->second(request.params());
      return JsonRpcResponse::success(result, request.id());
    } catch (const std::exception& e) {
      return JsonRpcResponse::error(JsonRpcResponse::INTERNAL_ERROR, e.what(),
                                   JsonValue::null(), request.id());
    }
  }

  // Method not found
  return JsonRpcResponse::error(JsonRpcResponse::METHOD_NOT_FOUND,
                               "Method not found: " + method,
                               JsonValue::null(), request.id());
}

std::string JsonRpcServer::handle(const std::string& json) {
  try {
    JsonRpcRequest request = JsonRpcRequest::parse(json);
    JsonRpcResponse response = handle(request);
    return response.stringify();
  } catch (const std::exception& e) {
    JsonRpcResponse response = JsonRpcResponse::error(
        JsonRpcResponse::PARSE_ERROR, e.what());
    return response.stringify();
  }
}

void JsonRpcServer::handleAsync(const JsonRpcRequest& request,
                                std::function<void(const JsonRpcResponse&)> callback) {
  const std::string& method = request.method();

  // Check async handlers
  auto it = async_handlers_.find(method);
  if (it != async_handlers_.end()) {
    try {
      JsonValue id = request.id();
      it->second(request.params(), [callback, id](const JsonValue& result) {
        callback(JsonRpcResponse::success(result, id));
      });
    } catch (const std::exception& e) {
      callback(JsonRpcResponse::error(JsonRpcResponse::INTERNAL_ERROR, e.what(),
                                     JsonValue::null(), request.id()));
    }
    return;
  }

  // Try sync handler
  auto sync_it = handlers_.find(method);
  if (sync_it != handlers_.end()) {
    callback(handle(request));
    return;
  }

  // Method not found
  callback(JsonRpcResponse::error(JsonRpcResponse::METHOD_NOT_FOUND,
                                 "Method not found: " + method,
                                 JsonValue::null(), request.id()));
}

} // namespace json
} // namespace mcp
} // namespace pbrlab