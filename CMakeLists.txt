cmake_minimum_required(VERSION 3.5)

set(BUILD_TARGET "pbrlab")

project(${BUILD_TARGET} C CXX)

# options
option(PBRJLAB_USE_CCACHE "Use ccache for faster recompile." ON)
#option(PBRLAB_WITH_OPENSUBDIV "Enable OpenSubdiv support for subdivision surface." ON)
#option(PBRLAB_WITH_OPENGL "Enable OpenGL rendering(off = use raytracing visualization with Embree)." ON)
option(PBRLAB_WITH_EMBREE "Use Embree for faster ray tracing." ON)

# cmake modules
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
# list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/sanitizers)
find_package(Sanitizers) # Address sanitizer (-DSANITIZE_ADDRESS=ON)
set(CMAKE_CXX_STANDARD 11)

# [ccache]
if(PBRLAB_USE_CCACHE)
  if(MSVC)
    # No ccache support
  else()
    find_program(CCACHE_EXE ccache)
    if(CCACHE_EXE)

      message(STATUS "Use ccache : " ${CCACHE_EXE})
      # CMAKE_C_COMPILER_LAUNCHER = available from cmake 3.4+)
      if(CMAKE_C_COMPILER_LAUNCHER)
        set(CMAKE_C_COMPILER_LAUNCHER "${CMAKE_C_COMPILER_LAUNCHER}"
                                      "${CCACHE_EXE}")
      else()
        set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_EXE}")
      endif()

      if(CMAKE_CXX_COMPILER_LAUNCHER)
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CMAKE_CXX_COMPILER_LAUNCHER}"
                                        "${CCACHE_EXE}")
      else()
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_EXE}")
      endif()

    endif(CCACHE_EXE)
  endif()
endif()


find_package(OpenGL REQUIRED)


# [glfw3]
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "" FORCE)
set(GLFW_INSTALL OFF CACHE INTERNAL "" FORCE)
add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/glfw)

# [Embree3]
if (PBRLAB_WITH_EMBREE)
  set(EMBREE_TASKING_SYSTEM Internal CACHE INTERNAL "" FORCE)
  set(EMBREE_ISPC_SUPPORT Off CACHE INTERNAL "" FORCE)
  set(EMBREE_TUTORIALS Off CACHE INTERNAL "" FORCE)
  set(EMBREE_MAX_ISA SSE2 CACHE INTERNAL "" FORCE)
  #set(EMBREE_MAX_ISA AVX CACHE INTERNAL "" FORCE)
  set(EMBREE_RAY_PACKETS Off CACHE INTERNAL "" FORCE)
  set(EMBREE_BUILD_VERIFY Off CACHE INTERNAL "" FORCE)

  if (SANITIZE_ADDRESS)
    set(EMBREE_ADDRESS_SANITIZER On CACHE INTERNAL "" FORCE)
    message(STATUS "Enable ASAN for Embree3")
  endif ()
  
  add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/embree-aarch64)
  list(APPEND PBRLAB_EXT_LIBRARIES embree)
endif (PBRLAB_WITH_EMBREE)

set(PBRLAB_SOURCES
    src/main.cc
    src/tinyexr.cc
    src/tiny_obj_loader.cc
    )

set(PBRLAB_DEP_SOURCES
    src/trackball.cc
    deps/glad/src/glad.c
    deps/imgui/imgui.cpp
    deps/imgui/imgui_draw.cpp
    deps/imgui/imgui_widgets.cpp
    deps/imgui/imgui_impl_opengl3.cpp
    deps/imgui/imgui_impl_glfw.cpp
    #deps/ImGuizmo/ImGuizmo.cpp
    #third_party/tinyxml2/tinyxml2.cpp
    )

list(APPEND PBRLAB_DEP_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party/StaticJSON/src/staticjson.cpp)

add_executable(${BUILD_TARGET} ${PBRLAB_SOURCES} ${PBRLAB_DEP_SOURCES})
add_sanitizers(${BUILD_TARGET})

target_include_directories(${BUILD_TARGET} PRIVATE ${PROJECT_SOURCE_DIR}/src)

# It looks like OpenSubdiv changes OUTPUT_DIRECTORY, so explicity specify output path for pbrlab
set_target_properties(${BUILD_TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# [embree3]
if (PBRLAB_WITH_EMBREE)
  target_include_directories(${BUILD_TARGET} PRIVATE  ${PROJECT_SOURCE_DIR}/third_party/embree-aarch64/include)
endif (PBRLAB_WITH_EMBREE)

# [imgui]
target_include_directories(${BUILD_TARGET} PRIVATE "${PROJECT_SOURCE_DIR}/deps/")

# [glad]
add_definitions("-DIMGUI_IMPL_OPENGL_LOADER_GLAD")
target_include_directories(${BUILD_TARGET} PRIVATE "${PROJECT_SOURCE_DIR}/deps/glad/include")

target_include_directories(${BUILD_TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/StaticJSON/include)
target_include_directories(${BUILD_TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/rapidjson/include)
target_include_directories(${BUILD_TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/toml11)


target_include_directories(${BUILD_TARGET} PRIVATE ${glfw_INCLUDE_DIRS})
target_link_libraries(${BUILD_TARGET} ${PBRLAB_EXT_LIBRARIES} glfw ${OPENGL_LIBRARIES} ${CMAKE_DL_LIBS})

# Increase warning level for clang.
IF (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set_source_files_properties(${PBRLAB_SOURCES} PROPERTIES COMPILE_FLAGS "-Weverything -Wno-system-headers -Werror -Wno-padded -Wno-c++98-compat-pedantic -Wno-documentation -Wno-documentation-unknown-command -Wno-reserved-id-macro -Wno-unused-variable -Wno-unused-function")
ENDIF ()


# [VisualStudio]
if(WIN32)
  # Set ${BUILD_TARGET} as a startup project for VS IDE
  set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${BUILD_TARGET})

  # For easier debugging in VS IDE(cmake 3.8.0 or later required) Set working
  # directory to ${BUILD_TARGET} git repo root.
  if(CMAKE_VERSION VERSION_GREATER 3.8.0)
    set_target_properties(${BUILD_TARGET}
                          PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY
                                     "${CMAKE_CURRENT_SOURCE_DIR}")
  endif()
endif()
