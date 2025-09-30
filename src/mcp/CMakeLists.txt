cmake_minimum_required(VERSION 3.16)

# MCP (Model Context Protocol) library
set(MCP_SOURCES
  json/json.cc
  json/json_rpc.cc
  core/mcp_protocol.cc
  core/mcp_server.cc
  transport/stdio_transport.cc
  transport/http_server.cc
  tools/render_tools.cc
)

set(MCP_HEADERS
  json/json.h
  json/json_rpc.h
  core/mcp_protocol.h
  core/mcp_server.h
  transport/stdio_transport.h
  transport/http_server.h
  tools/render_tools.h
)

# Create MCP library
add_library(pbrlab_mcp STATIC ${MCP_SOURCES} ${MCP_HEADERS})

target_include_directories(pbrlab_mcp PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Platform-specific networking libraries
if(WIN32)
  target_link_libraries(pbrlab_mcp PUBLIC ws2_32)
endif()

if(UNIX AND NOT APPLE)
  find_package(Threads REQUIRED)
  target_link_libraries(pbrlab_mcp PUBLIC Threads::Threads)
endif()

if(APPLE)
  find_package(Threads REQUIRED)
  target_link_libraries(pbrlab_mcp PUBLIC Threads::Threads)
endif()

# MCP Server executable
add_executable(pbrlab-mcp-server pbrlab_mcp_server.cc)
target_link_libraries(pbrlab-mcp-server PRIVATE pbrlab_mcp)

# Set C++ standard
set_target_properties(pbrlab_mcp pbrlab-mcp-server PROPERTIES
  CXX_STANDARD 11
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)

# Install targets
install(TARGETS pbrlab-mcp-server
  RUNTIME DESTINATION bin
)

install(TARGETS pbrlab_mcp
  ARCHIVE DESTINATION lib
)

install(FILES ${MCP_HEADERS}
  DESTINATION include/pbrlab/mcp
)