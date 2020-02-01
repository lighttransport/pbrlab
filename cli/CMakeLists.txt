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


set(EXECUTABLE_TARGETS "")

add_executable(pbrlab-cli ./pbrlab-cli.cc)
list(APPEND EXECUTABLE_TARGETS pbrlab-cli)


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
    ${PROJECT_NAME})

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
