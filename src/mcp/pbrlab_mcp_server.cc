#include "core/mcp_server.h"
#include "tools/render_tools.h"
#include <iostream>
#include <string>
#include <cstring>
#include <chrono>
#include <thread>
#include <cstdlib>

using namespace pbrlab::mcp;

void printUsage(const char* program) {
  std::cout << "Usage: " << program << " [options]\n";
  std::cout << "Options:\n";
  std::cout << "  --stdio         Use stdio transport (default)\n";
  std::cout << "  --http [port]   Use HTTP transport (default port: 8080)\n";
  std::cout << "  --sse [port]    Use HTTP/SSE transport (default port: 8080)\n";
  std::cout << "  --help          Show this help message\n";
}

int main(int argc, char* argv[]) {
  McpServer::TransportType transport = McpServer::TransportType::Stdio;
  int port = 8080;

  // Parse command line arguments
  for (int i = 1; i < argc; i++) {
    if (std::strcmp(argv[i], "--stdio") == 0) {
      transport = McpServer::TransportType::Stdio;
    } else if (std::strcmp(argv[i], "--http") == 0) {
      transport = McpServer::TransportType::Http;
      if (i + 1 < argc && argv[i + 1][0] != '-') {
        port = std::atoi(argv[++i]);
      }
    } else if (std::strcmp(argv[i], "--sse") == 0) {
      transport = McpServer::TransportType::HttpSSE;
      if (i + 1 < argc && argv[i + 1][0] != '-') {
        port = std::atoi(argv[++i]);
      }
    } else if (std::strcmp(argv[i], "--help") == 0) {
      printUsage(argv[0]);
      return 0;
    } else {
      std::cerr << "Unknown option: " << argv[i] << "\n";
      printUsage(argv[0]);
      return 1;
    }
  }

  // Create and configure MCP server
  McpServer server("pbrlab-mcp", "1.0.0");

  // Enable capabilities
  server.enableTools();
  server.enableResources();
  server.enablePrompts();

  // Register rendering tools
  tools::registerRenderTools(server);

  // Register example resources
  server.registerResource(
    "pbrlab://docs/readme",
    "PBRLab README",
    "Main documentation for PBRLab",
    "text/markdown",
    [](const std::string& uri) -> std::string {
      return "# PBRLab MCP Server\n\n"
             "This is the Model Context Protocol server for PBRLab.\n\n"
             "## Features\n"
             "- Path tracing rendering\n"
             "- Material editing\n"
             "- Scene management\n"
             "- Camera control\n";
    });

  server.registerResource(
    "pbrlab://config/default",
    "Default Configuration",
    "Default render configuration",
    "application/json",
    [](const std::string& uri) -> std::string {
      return "{\n"
             "  \"width\": 1920,\n"
             "  \"height\": 1080,\n"
             "  \"samples\": 128,\n"
             "  \"max_depth\": 8\n"
             "}";
    });

  // Register example prompts
  server.registerPrompt(
    "render_scene",
    "Generate a render command for a scene",
    {"scene_file", "quality"},
    [](const json::JsonValue& args) -> std::string {
      std::string scene = args.has("scene_file") ?
                         args.at("scene_file").asString() : "scene.obj";
      std::string quality = args.has("quality") ?
                          args.at("quality").asString() : "medium";

      int samples = 64;
      if (quality == "low") samples = 16;
      else if (quality == "high") samples = 256;
      else if (quality == "ultra") samples = 1024;

      return "Render the scene '" + scene + "' with " +
             std::to_string(samples) + " samples per pixel for " +
             quality + " quality output.";
    });

  // Start the server
  std::cout << "Starting PBRLab MCP Server...\n";

  switch (transport) {
    case McpServer::TransportType::Stdio:
      std::cout << "Using stdio transport\n";
      std::cout << "Ready for MCP messages on stdin/stdout\n";
      break;

    case McpServer::TransportType::Http:
      std::cout << "Using HTTP transport on port " << port << "\n";
      break;

    case McpServer::TransportType::HttpSSE:
      std::cout << "Using HTTP/SSE transport on port " << port << "\n";
      break;
  }

  if (!server.start(transport, port)) {
    std::cerr << "Failed to start MCP server\n";
    return 1;
  }

  if (transport == McpServer::TransportType::Http ||
      transport == McpServer::TransportType::HttpSSE) {
    std::cout << "Server running at: " << server.getHttpAddress() << "/mcp\n";
    if (transport == McpServer::TransportType::HttpSSE) {
      std::cout << "SSE events at: " << server.getHttpAddress() << "/mcp/events\n";
    }
    std::cout << "Press Enter to stop...\n";
    std::cin.get();
  } else {
    // Stdio transport runs until terminated
    while (server.isRunning()) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

  server.stop();
  std::cout << "MCP server stopped\n";

  return 0;
}