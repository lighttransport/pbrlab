#ifndef PBRLAB_MCP_JSON_RPC_H_
#define PBRLAB_MCP_JSON_RPC_H_

#include "json.h"
#include <functional>
#include <memory>

namespace pbrlab {
namespace mcp {
namespace json {

class JsonRpcRequest {
public:
  JsonRpcRequest() = default;
  JsonRpcRequest(const std::string& method, const JsonValue& params = JsonValue::null(),
                 const JsonValue& id = JsonValue::null())
      : method_(method), params_(params), id_(id) {}

  static JsonRpcRequest parse(const std::string& json);
  static JsonRpcRequest parse(const JsonValue& json);

  const std::string& method() const { return method_; }
  const JsonValue& params() const { return params_; }
  const JsonValue& id() const { return id_; }

  bool isNotification() const { return id_.isNull(); }

  JsonValue toJson() const;
  std::string stringify() const;

private:
  std::string method_;
  JsonValue params_;
  JsonValue id_;
};

class JsonRpcResponse {
public:
  JsonRpcResponse() = default;

  static JsonRpcResponse success(const JsonValue& result, const JsonValue& id);
  static JsonRpcResponse error(int code, const std::string& message,
                              const JsonValue& data = JsonValue::null(),
                              const JsonValue& id = JsonValue::null());

  static JsonRpcResponse parse(const std::string& json);
  static JsonRpcResponse parse(const JsonValue& json);

  bool isSuccess() const { return !result_.isNull() && error_.isNull(); }
  bool isError() const { return !error_.isNull(); }

  const JsonValue& result() const { return result_; }
  const JsonValue& error() const { return error_; }
  const JsonValue& id() const { return id_; }

  JsonValue toJson() const;
  std::string stringify() const;

  // Standard error codes
  static constexpr int PARSE_ERROR = -32700;
  static constexpr int INVALID_REQUEST = -32600;
  static constexpr int METHOD_NOT_FOUND = -32601;
  static constexpr int INVALID_PARAMS = -32602;
  static constexpr int INTERNAL_ERROR = -32603;

private:
  JsonRpcResponse(const JsonValue& result, const JsonValue& error, const JsonValue& id)
      : result_(result), error_(error), id_(id) {}

  JsonValue result_;
  JsonValue error_;
  JsonValue id_;
};

class JsonRpcNotification {
public:
  JsonRpcNotification(const std::string& method, const JsonValue& params = JsonValue::null())
      : method_(method), params_(params) {}

  const std::string& method() const { return method_; }
  const JsonValue& params() const { return params_; }

  JsonValue toJson() const;
  std::string stringify() const;

  static JsonRpcNotification parse(const std::string& json);
  static JsonRpcNotification parse(const JsonValue& json);

private:
  std::string method_;
  JsonValue params_;
};

using JsonRpcHandler = std::function<JsonValue(const JsonValue& params)>;
using JsonRpcAsyncHandler = std::function<void(const JsonValue& params,
                                               std::function<void(const JsonValue&)> callback)>;

class JsonRpcServer {
public:
  JsonRpcServer() = default;

  void registerMethod(const std::string& method, JsonRpcHandler handler);
  void registerAsyncMethod(const std::string& method, JsonRpcAsyncHandler handler);

  JsonRpcResponse handle(const JsonRpcRequest& request);
  std::string handle(const std::string& json);

  void handleAsync(const JsonRpcRequest& request,
                   std::function<void(const JsonRpcResponse&)> callback);

private:
  std::map<std::string, JsonRpcHandler> handlers_;
  std::map<std::string, JsonRpcAsyncHandler> async_handlers_;
};

} // namespace json
} // namespace mcp
} // namespace pbrlab

#endif // PBRLAB_MCP_JSON_RPC_H_