# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

pbrlab is a path tracing + PBR (Physically Based Rendering) shading/rendering implementation in portable C++11. It's primarily used for verifying renderers and PBR shading implementations with well-tested algorithms including Disney's PrincipledBSDF with subsurface scattering and Hair BSDF.

## Build Commands

### Linux/macOS
```bash
# Full bootstrap with all build types
./scripts/bootstrap-linux.sh    # Linux
./scripts/bootstrap-macos.sh    # macOS

# Build after bootstrap
cd cmake-build-relwithdebinfo
make -j

# CLI-only build (no GUI)
./scripts/bootstrap-linux-cli.sh

# Build with external Embree
./scripts/bootstrap-linux-external-embree.sh
```

### Windows
```bash
# Generate Visual Studio solution
vcsetup-2022.bat
# Then open build/pbrlab.sln in Visual Studio
```

### Manual CMake
```bash
mkdir build
cmake -B build -S . -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

### Key CMake Options
- `PBRLAB_BUILD_GUI`: Build GUI version (default: OFF)
- `PBRLAB_WITH_EMBREE`: Use Embree for ray tracing (default: ON, otherwise uses NanoRT)
- `PBRLAB_USE_EXTERNAL_EMBREE`: Link external Embree instead of submodule
- `PBRLAB_USE_CCACHE`: Use ccache for faster rebuilds (default: ON)
- `EMBREE_ARM`: Required for ARM targets

## Running the Application

```bash
# Demo scene
./pbrlab data/cornellbox_suzanne_lucy.obj

# Surface mesh only
./pbrlab input.obj

# Hair only (cyhair format)
./pbrlab input.hair

# Mesh and hair combined
./pbrlab input.obj input.hair
```

## Architecture Overview

### Core Components

1. **Rendering Engine** (`src/render.h`, `src/render.cc`)
   - Path tracing implementation
   - Multi-threaded rendering with atomic controls
   - Render layers and pass management

2. **BSDF/Shading** (`src/closure/`)
   - PrincipledBSDF implementation with Disney model
   - Random-walk subsurface scattering
   - Principled Hair BSDF (Cycles/pbrt-v3 based)
   - Microfacet models (GGX)

3. **Scene Management** (`src/scene.h`, `src/nanosg.h`)
   - Scene graph representation
   - Light management (`src/light-manager.h`)
   - Material parameters (`src/material-param.h`)

4. **Ray Tracing Backends**
   - Embree integration (when `PBRLAB_WITH_EMBREE=ON`)
   - NanoRT fallback implementation (`src/nanort.h`)

5. **I/O** (`src/io/`)
   - Wavefront OBJ/MTL loader with PBR extensions
   - CyHair format support for hair/fur
   - Scene description formats

6. **GUI Application** (`pc/`)
   - `pbrlab-gui`: OpenGL/ImGui-based interactive viewer
   - `pbrlab-cli`: Command-line renderer
   - GLFW window management

### Material System

The material system extends Wavefront MTL format with PBR and SSS parameters compatible with Arnold Standard Surface:

- Base material: PrincipledBSDF parameters
- Subsurface scattering: `subsurface`, `subsurface_radius`, `subsurface_color`
- Hair: Separate Hair BSDF with Cycles-compatible parameters

### Build Outputs

- `pbrlab-gui`: Interactive GUI application (when `PBRLAB_BUILD_GUI=ON`)
- `pbrlab-cli`: Command-line renderer
- `pbrlab-mcp-server`: Model Context Protocol server for AI integration
- Build directories: `cmake-build-{debug,relwithdebinfo,release}`

### MCP (Model Context Protocol) Server

PBRLab now includes an MCP server that provides AI-friendly interfaces for rendering operations:

```bash
# Run with stdio transport (for direct AI integration)
./src/mcp/pbrlab-mcp-server --stdio

# Run with HTTP transport
./src/mcp/pbrlab-mcp-server --http 8080

# Run with HTTP/SSE for streaming
./src/mcp/pbrlab-mcp-server --sse 8080
```

The MCP server exposes tools for:
- Starting and monitoring render jobs
- Loading and managing scenes
- Setting material parameters
- Controlling camera position
- Exporting rendered images

Implementation details:
- Pure C++11 JSON and JSON-RPC implementation (no external dependencies)
- Cross-platform HTTP server (Winsock2 on Windows, POSIX sockets on Unix)
- Support for stdio, HTTP, and HTTP/SSE transports
- Located in `src/mcp/`

## Dependencies

### Required
- C++11 compiler (clang++ recommended)
- CMake 3.16+
- Threads support

### Optional/Included
- Embree (submodule via embree-portable or external)
- OpenGL 3.x + development packages (for GUI)
- GLFW3 (submodule)
- ImGui, TinyObjLoader, TinyEXR, stb libraries (included)

## Development Notes

- The codebase uses namespace `pbrlab` throughout
- Atomic operations are used for render cancellation and progress tracking
- Multiple build configurations are maintained simultaneously
- Embree-portable is preferred over system Embree for consistency