# find package and check third party
# - google glog (optional)

# Stack Trace logger (google glog)
if(PBRLAB_USE_STACK_TRACE_LOGGER)
    message(STATUS "Stack trace logger: ENABLED")
    # Glog
    find_package(Glog REQUIRED)
else()
    message(STATUS "Stack trace logger: DISABLED")
endif()

set(PBRLAB_GUI_DEPS_INCLUDE_DIRS
  ${PROJECT_SOURCE_DIR}/deps
  #${PROJECT_SOURCE_DIR}/deps/glad/include
)
set(PBRLAB_GUI_DEPS_SOURCES
  #${PROJECT_SOURCE_DIR}/deps/glad/src/glad.c
  ${PROJECT_SOURCE_DIR}/deps/imgui/imgui.cpp
  ${PROJECT_SOURCE_DIR}/deps/imgui/imgui_draw.cpp
  ${PROJECT_SOURCE_DIR}/deps/imgui/imgui_widgets.cpp
  ${PROJECT_SOURCE_DIR}/deps/imgui/imgui_tables.cpp
  ${PROJECT_SOURCE_DIR}/deps/imgui/imgui_impl_opengl3.cpp
  ${PROJECT_SOURCE_DIR}/deps/imgui/imgui_impl_glfw.cpp
  ${PROJECT_SOURCE_DIR}/deps/ImGuizmo/ImGuizmo.cpp
)
add_library(${BUILD_TARGET}-gui-deps STATIC ${PBRLAB_GUI_DEPS_SOURCES})
target_include_directories(${BUILD_TARGET}-gui-deps PUBLIC ${PBRLAB_GUI_DEPS_INCLUDE_DIRS})
target_compile_features(${BUILD_TARGET}-gui-deps PRIVATE cxx_std_11)
target_link_libraries(${BUILD_TARGET}-gui-deps PUBLIC glfw ${OPENGL_LIBRARIES})
#target_compile_definitions(${BUILD_TARGET}-gui-deps PUBLIC "-DIMGUI_IMPL_OPENGL_LOADER_GLAD")


set(EXECUTABLE_TARGETS "")

add_executable(${BUILD_TARGET}-cli ./pbrlab-cli.cc
                                   ./pc-common.cc)
list(APPEND EXECUTABLE_TARGETS ${BUILD_TARGET}-cli)

add_executable(${BUILD_TARGET}-gui pbrlab-gui.cc
                                   glfw-window.cc
                                   pc-common.cc)
list(APPEND EXECUTABLE_TARGETS ${BUILD_TARGET}-gui)
target_link_libraries(${BUILD_TARGET}-gui PRIVATE ${BUILD_TARGET}-gui-deps)


foreach(EXECUTABLE_TARGET IN LISTS EXECUTABLE_TARGETS)
  add_sanitizers(${EXECUTABLE_TARGET})
  target_compile_features(${EXECUTABLE_TARGET} PRIVATE cxx_std_11)

  # set output directory for executables
  set_target_properties(${EXECUTABLE_TARGET} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

  target_compile_options(${EXECUTABLE_TARGET}
    PRIVATE
    $<$<CXX_COMPILER_ID:Clang>: ${PBRLAB_CLANG_COMPILE_OPTIONS}>
    $<$<CXX_COMPILER_ID:GCC>:-Wall -Wextra>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>)

  target_link_libraries(${EXECUTABLE_TARGET} PRIVATE
    ${PROJECT_NAME}
    mpark_variant)

  if (PBRLAB_USE_STACK_TRACE_LOGGER)
    target_compile_definitions(${EXECUTABLE_TARGET}
      PRIVATE -DPBRLAB_USE_STACK_TRACE_LOGGER)
    target_link_libraries(${EXECUTABLE_TARGET} PRIVATE glog::glog)
  endif()

  # [VisualStudio]
  if(WIN32)
    # Set ${BUILD_TARGET} as a startup project for VS IDE
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${EXECUTABLE_TARGET})
  
    # For easier debugging in VS IDE(cmake 3.8.0 or later required) Set working
    # directory to ${BUILD_TARGET} git repo root.
    if(CMAKE_VERSION VERSION_GREATER 3.8.0)
      set_target_properties(${EXECUTABLE_TARGET}
                            PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY
                                       "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()
  endif()

endforeach()
