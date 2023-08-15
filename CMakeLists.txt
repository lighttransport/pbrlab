cmake_minimum_required(VERSION 3.16)

set(BUILD_TARGET "pbrlab")
project(${BUILD_TARGET} LANGUAGES C CXX)

# options
option(PBRLAB_USE_CCACHE "Use ccache for faster recompile." ON)
option(PBRLAB_BUILD_GUI "Build GUI." ON)
option(PBRLAB_USE_STACK_TRACE_LOGGER "Enable automatic stack trace logger of google-glog. You must install google glog on your computer if you enable this option." OFF)
option(PBRLAB_WITH_EMBREE "Use Embree submodule for faster ray tracing(Off = use NanoRT(not implemented yet))." ON)
option(PBRLAB_WITH_TINYUSDZ "Use TinyUSDZ submodule to support reading USD scene." ON)
option(PBRLAB_USE_EXTERNAL_EMBREE "Use Embree(by linking externally) for faster ray tracing(Need to set path to Embree cmake config)." OFF)

# cmake modules
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
# list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/sanitizers)
find_package(Sanitizers) # Address sanitizer (-DSANITIZE_ADDRESS=ON)

# Overall compile options
#set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_GLIBCXX_DEBUG -D_LIBCPP_DEBUG")
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_GLIBCXX_DEBUG -D_LIBCPP_DEBUG")

if (PBRLAB_WITH_TINYUSDZ)
  set(CMAKE_CXX_STANDARD 14)
else()
  set(CMAKE_CXX_STANDARD 11)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

set(PBRLAB_EXT_INCLUDE_DIRS_PRIVATE "")
set(PBRLAB_EXT_COMPLE_DEFINITIONS_PRIVATE "")
set(PBRLAB_EXT_LIBRARIES_PRIVATE "")
set(PBRLAB_EXT_LIBRARIES_PUBLIC "")

# find package and check third party
# - pthread
# - OpenGL
# - glfw3
# - Embree3
# - mpark variant
# - ghc filesystem
# - StaticJson
# TODO check third_party

# [Threads]
find_package(Threads REQUIRED)
if(NOT Threads_FOUND)
  message(FATAL_ERROR "Threads not found")
endif()

if (PBRLAB_BUILD_GUI)
  # [OpenGL]
  find_package(OpenGL REQUIRED)

  # [glfw3]
  set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "" FORCE)
  set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "" FORCE)
  set(GLFW_INSTALL OFF CACHE INTERNAL "" FORCE)
  add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/glfw)
endif()

# [Embree3]
if (PBRLAB_WITH_EMBREE)
  if (NOT PBRLAB_USE_EXTERNAL_EMBREE)
    set(EMBREE_TASKING_SYSTEM Internal CACHE INTERNAL "" FORCE)
    set(EMBREE_ISPC_SUPPORT Off CACHE INTERNAL "" FORCE)
    set(EMBREE_TUTORIALS Off CACHE INTERNAL "" FORCE)
    set(EMBREE_MAX_ISA SSE2 CACHE INTERNAL "" FORCE)
    #set(EMBREE_MAX_ISA AVX CACHE INTERNAL "" FORCE)
    set(EMBREE_RAY_PACKETS Off CACHE INTERNAL "" FORCE)
    set(EMBREE_BUILD_VERIFY Off CACHE INTERNAL "" FORCE)

    # Use static build
    set(EMBREE_STATIC_LIB On CACHE INTERNAL "" FORCE)

    # Use pbrlab's compile flags.
    set(EMBREE_USE_PARENT_PROJECT_COMPILER_FLAGS On CACHE INTERNAL "" FORCE)

    if (SANITIZE_ADDRESS)
      set(EMBREE_ADDRESS_SANITIZER On CACHE INTERNAL "" FORCE)
      message(STATUS "Enable ASAN for Embree3")
    endif ()
    
    add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/embree-aarch64)
    list(APPEND PBRLAB_EXT_LIBRARIES_PRIVATE embree)
    list(APPEND PBRLAB_EXT_INCLUDE_DIRS_PRIVATE ${CMAKE_SOURCE_DIR}/third_party/embree-aarch64/include)
  else()
    message(STATUS "Searching external embree...")
    find_package(embree 3.12 REQUIRED)
    if (embree_FOUND)
      message("External Embree found in ${EMBREE_INCLUDE_DIRS}")
    else()
      message(FATAL_ERROR "External Embree not found")
    endif()
    list(APPEND PBRLAB_EXT_LIBRARIES_PRIVATE ${EMBREE_LIBRARY})
    list(APPEND PBRLAB_EXT_INCLUDE_DIRS_PRIVATE ${EMBREE_INCLUDE_DIRS})
  endif ()
endif (PBRLAB_WITH_EMBREE)

# [mpark variant]
add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/variant)
list(APPEND PBRLAB_EXT_LIBRARIES_PUBLIC mpark_variant)

# [ghc filesystem]
add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/filesystem)
list(APPEND PBRLAB_EXT_LIBRARIES_PUBLIC ghc_filesystem)

# [Static JSON]
add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/)
list(APPEND PBRLAB_EXT_LIBRARIES_PRIVATE staticjson)

if (PBRLAB_WITH_TINYUSDZ)
  add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/tinyusdz tinyusdz)
  list(APPEND PBRLAB_EXT_LIBRARIES_PRIVATE tinyusdz_static)
  list(APPEND PBRLAB_EXT_INCLUDE_DIRS_PRIVATE ${PROJECT_SOURCE_DIR}/third_paty/tinyusdz/src)
  list(APPEND PBRLAB_EXT_COMPILE_DEFINITIONS_PRIVATE "-DPBRLAB_USE_TINYUSDZ=1 ")
  message(STATUS "Enable USD support through TinyUSDZ")
endif()

set(PBRLAB_CLANG_COMPILE_OPTIONS -Wall -Wextra -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded)

add_subdirectory(src)
add_subdirectory(pc)

# [VisualStudio]
if(MSVC)

  # For easier debugging in VS IDE(cmake 3.8.0 or later required) Set working
  # directory to ${BUILD_TARGET} git repo root.
  if(CMAKE_VERSION VERSION_GREATER 3.8.0)
    set_target_properties(pbrlab-cli
                             PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY
                                      "${CMAKE_CURRENT_SOURCE_DIR}")
  endif()

  # For easier debugging in VS IDE, set a demo scene. 
  if(CMAKE_VERSION VERSION_GREATER 3.13.0)
      set_target_properties(pbrlab-cli
                            PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS
                                       "./data/cornellbox_suzanne_lucy.obj")
  endif()

  if (PBRLAB_BUILD_GUI)

    # For easier debugging in VS IDE(cmake 3.8.0 or later required) Set working
    # directory to ${BUILD_TARGET} git repo root.
    if(CMAKE_VERSION VERSION_GREATER 3.8.0)
      set_target_properties(pbrlab-gui
                              PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY
                                         "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()

    # For easier debugging in VS IDE, set a demo scene. 
    if(CMAKE_VERSION VERSION_GREATER 3.13.0)
        set_target_properties(pbrlab-gui
                              PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS
                                         "./data/cornellbox_suzanne_lucy.obj")
    endif()

    # Set startup project for VS IDE
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT pbrlab-gui)
  else()
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT pbrlab-cli)
  endif()
endif()

