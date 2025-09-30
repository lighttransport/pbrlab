#include "render_tools.h"
#include <fstream>
#include <sstream>
#include <cmath>

namespace pbrlab {
namespace mcp {
namespace tools {

void registerRenderTools(McpServer& server) {
  // Enable tools capability
  server.enableTools();

  // Tool: Start Render
  {
    auto schema = SchemaBuilder::objectWithProperties({
        {"scene_file", SchemaBuilder::string("Path to the scene file (.obj, .hair)")},
        {"width", SchemaBuilder::number("Render width in pixels")},
        {"height", SchemaBuilder::number("Render height in pixels")},
        {"samples", SchemaBuilder::number("Number of samples per pixel")},
        {"output_file", SchemaBuilder::string("Output image file path")}
      },
      {"scene_file", "width", "height", "samples"}  // required fields
    );

    server.registerTool("render.start",
      "Start a path tracing render job",
      schema,
      [](const json::JsonValue& args) -> ToolResult {
        ToolResult result;

        try {
          std::string scene_file = args.at("scene_file").asString();
          int width = args.at("width").asInt();
          int height = args.at("height").asInt();
          int samples = args.at("samples").asInt();

          std::string output_file = "output.png";
          if (args.has("output_file")) {
            output_file = args.at("output_file").asString();
          }

          // TODO: Integrate with actual pbrlab render engine
          json::JsonValue response = json::JsonValue::object();
          response["job_id"] = json::JsonValue("render_" + std::to_string(std::rand()));
          response["status"] = json::JsonValue("started");
          response["scene"] = json::JsonValue(scene_file);
          response["resolution"] = json::JsonValue(std::to_string(width) + "x" + std::to_string(height));
          response["samples"] = json::JsonValue(samples);
          response["output"] = json::JsonValue(output_file);

          result.success = true;
          result.content = response;
        } catch (const std::exception& e) {
          result.success = false;
          result.isError = true;
          result.content = json::JsonValue(std::string("Error: ") + e.what());
        }

        return result;
      });
  }

  // Tool: Get Render Status
  {
    auto schema = SchemaBuilder::objectWithProperties({
        {"job_id", SchemaBuilder::string("Render job ID")}
      },
      {"job_id"}
    );

    server.registerTool("render.status",
      "Get the status of a render job",
      schema,
      [](const json::JsonValue& args) -> ToolResult {
        ToolResult result;

        try {
          std::string job_id = args.at("job_id").asString();

          // TODO: Query actual render status
          json::JsonValue response = json::JsonValue::object();
          response["job_id"] = json::JsonValue(job_id);
          response["status"] = json::JsonValue("in_progress");
          response["progress"] = json::JsonValue(0.42);  // 42% complete
          response["estimated_time_remaining"] = json::JsonValue("2 minutes");

          result.success = true;
          result.content = response;
        } catch (const std::exception& e) {
          result.success = false;
          result.isError = true;
          result.content = json::JsonValue(std::string("Error: ") + e.what());
        }

        return result;
      });
  }

  // Tool: Load Scene
  {
    auto schema = SchemaBuilder::objectWithProperties({
        {"file_path", SchemaBuilder::string("Path to the scene file")}
      },
      {"file_path"}
    );

    server.registerTool("scene.load",
      "Load a 3D scene file",
      schema,
      [](const json::JsonValue& args) -> ToolResult {
        ToolResult result;

        try {
          std::string file_path = args.at("file_path").asString();

          // Check if file exists
          std::ifstream file(file_path);
          if (!file.good()) {
            throw std::runtime_error("File not found: " + file_path);
          }

          // TODO: Actually load the scene
          json::JsonValue response = json::JsonValue::object();
          response["file"] = json::JsonValue(file_path);
          response["status"] = json::JsonValue("loaded");

          // Dummy scene info
          json::JsonValue scene_info = json::JsonValue::object();
          scene_info["vertices"] = json::JsonValue(12345);
          scene_info["triangles"] = json::JsonValue(4096);
          scene_info["materials"] = json::JsonValue(3);
          response["info"] = scene_info;

          result.success = true;
          result.content = response;
        } catch (const std::exception& e) {
          result.success = false;
          result.isError = true;
          result.content = json::JsonValue(std::string("Error: ") + e.what());
        }

        return result;
      });
  }

  // Tool: Set Material Parameters
  {
    auto schema = SchemaBuilder::objectWithProperties({
        {"material_name", SchemaBuilder::string("Name of the material")},
        {"parameter", SchemaBuilder::string("Parameter to set (e.g., 'metallic', 'roughness')")},
        {"value", SchemaBuilder::number("Parameter value")}
      },
      {"material_name", "parameter", "value"}
    );

    server.registerTool("material.set_parameter",
      "Set a material parameter value",
      schema,
      [](const json::JsonValue& args) -> ToolResult {
        ToolResult result;

        try {
          std::string material_name = args.at("material_name").asString();
          std::string parameter = args.at("parameter").asString();
          double value = args.at("value").asNumber();

          // Validate parameter range
          if (parameter == "metallic" || parameter == "roughness") {
            if (value < 0.0 || value > 1.0) {
              throw std::runtime_error(parameter + " must be between 0 and 1");
            }
          }

          // TODO: Actually set the material parameter
          json::JsonValue response = json::JsonValue::object();
          response["material"] = json::JsonValue(material_name);
          response["parameter"] = json::JsonValue(parameter);
          response["value"] = json::JsonValue(value);
          response["status"] = json::JsonValue("updated");

          result.success = true;
          result.content = response;
        } catch (const std::exception& e) {
          result.success = false;
          result.isError = true;
          result.content = json::JsonValue(std::string("Error: ") + e.what());
        }

        return result;
      });
  }

  // Tool: Camera Control
  {
    auto schema = SchemaBuilder::objectWithProperties({
        {"position", SchemaBuilder::array(SchemaBuilder::number(), "Camera position [x, y, z]")},
        {"target", SchemaBuilder::array(SchemaBuilder::number(), "Look-at target [x, y, z]")},
        {"fov", SchemaBuilder::number("Field of view in degrees")}
      },
      {}  // all optional
    );

    server.registerTool("camera.set",
      "Set camera position and orientation",
      schema,
      [](const json::JsonValue& args) -> ToolResult {
        ToolResult result;

        try {
          json::JsonValue response = json::JsonValue::object();

          if (args.has("position")) {
            const auto& pos = args.at("position").asArray();
            if (pos.size() != 3) {
              throw std::runtime_error("Position must be [x, y, z]");
            }
            response["position"] = args.at("position");
          }

          if (args.has("target")) {
            const auto& target = args.at("target").asArray();
            if (target.size() != 3) {
              throw std::runtime_error("Target must be [x, y, z]");
            }
            response["target"] = args.at("target");
          }

          if (args.has("fov")) {
            double fov = args.at("fov").asNumber();
            if (fov < 1.0 || fov > 179.0) {
              throw std::runtime_error("FOV must be between 1 and 179 degrees");
            }
            response["fov"] = json::JsonValue(fov);
          }

          response["status"] = json::JsonValue("updated");
          result.success = true;
          result.content = response;
        } catch (const std::exception& e) {
          result.success = false;
          result.isError = true;
          result.content = json::JsonValue(std::string("Error: ") + e.what());
        }

        return result;
      });
  }

  // Tool: Export Image
  {
    auto schema = SchemaBuilder::objectWithProperties({
        {"format", SchemaBuilder::string("Output format: 'png', 'jpg', 'exr'")},
        {"output_path", SchemaBuilder::string("Output file path")}
      },
      {"format", "output_path"}
    );

    server.registerTool("render.export",
      "Export the rendered image",
      schema,
      [](const json::JsonValue& args) -> ToolResult {
        ToolResult result;

        try {
          std::string format = args.at("format").asString();
          std::string output_path = args.at("output_path").asString();

          // Validate format
          if (format != "png" && format != "jpg" && format != "exr") {
            throw std::runtime_error("Unsupported format: " + format);
          }

          // TODO: Actually export the image
          json::JsonValue response = json::JsonValue::object();
          response["format"] = json::JsonValue(format);
          response["path"] = json::JsonValue(output_path);
          response["status"] = json::JsonValue("exported");
          response["size_bytes"] = json::JsonValue(1024 * 512);  // Dummy size

          result.success = true;
          result.content = response;
        } catch (const std::exception& e) {
          result.success = false;
          result.isError = true;
          result.content = json::JsonValue(std::string("Error: ") + e.what());
        }

        return result;
      });
  }
}

} // namespace tools
} // namespace mcp
} // namespace pbrlab