#ifndef PBRLAB_MCP_RENDER_TOOLS_H_
#define PBRLAB_MCP_RENDER_TOOLS_H_

#include "../core/mcp_server.h"

namespace pbrlab {
namespace mcp {
namespace tools {

// Register rendering-related MCP tools
void registerRenderTools(McpServer& server);

} // namespace tools
} // namespace mcp
} // namespace pbrlab

#endif // PBRLAB_MCP_RENDER_TOOLS_H_