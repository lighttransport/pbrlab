cmake_minimum_required(VERSION 3.5)

set(BUILD_TARGET "pbrlab")
project(${BUILD_TARGET} LANGUAGES C CXX)

# options
option(PBRLAB_USE_CCACHE "Use ccache for faster recompile." ON)
option(PBRLAB_USE_STACK_TRACE_LOGGER "Enable automatic stack trace logger of google-glog. You must install google glog on your computer if you enable this option." OFF)
option(PBRLAB_WITH_EMBREE "Use Embree for faster ray tracing." ON)

# cmake modules
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
# list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/sanitizers)
find_package(Sanitizers) # Address sanitizer (-DSANITIZE_ADDRESS=ON)

# Overall compile options
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_GLIBCXX_DEBUG -D_LIBCPP_DEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_GLIBCXX_DEBUG -D_LIBCPP_DEBUG")

set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g -D_GLIBCXX_DEBUG -D_LIBCPP_DEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -D_GLIBCXX_DEBUG -D_LIBCPP_DEBUG")


# Check C++11 support
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-std=c++11" COMPILER_SUPPORTS_CXX11)
if (COMPILER_SUPPORTS_CXX11)
    message(STATUS "C++11 support: OK (-std=c++11)")
else()
    message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

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


# find package and check third party
# - OpenGL
# - glfw3
# - Embree3
# - mpark variant
# - ghc filesystem
# TODO check third_party

# [OpenGL]
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

# [mpark variant]
add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/variant)
list(APPEND PBRLAB_EXT_LIBRARIES mpark_variant)

# [ghc filesystem]
add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/filesystem)
list(APPEND PBRLAB_EXT_LIBRARIES ghc_filesystem)


set(PBRLAB_CLANG_COMPILE_OPTIONS -Wall -Wextra -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded)

add_subdirectory(src)
add_subdirectory(cli)
