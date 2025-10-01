set(PBRLAB_SOURCES
    curve-util.cc
    image-utils.cc
    light-manager.cc
    matrix.cc
    render.cc
    render-layer.cc
    render-tile.cc
    scene.cc
    io/curve-mesh-io.cc
    io/cyhair.cc
    io/image-io.cc
    io/render-config-io.cc
    io/scene-description-io.cc
    io/stb_image.cc
    io/stb_image_write.cc
    io/tinyexr.cc
    io/tiny_obj_loader.cc
    io/triangle-mesh-io.cc
    mesh/triangle-mesh.cc
    mesh/cubic-bezier-curve-mesh.cc
    raytracer/raytracer.cc
    raytracer/raytracer_impl.cc
    raytracer/raytracer_tinybvh_impl.cc
    scene-description/scene-description.cc
    shader/cycles-principled-shader.cc
    shader/hair-shader.cc
    shader/shader.cc
    texture.cc
    miniz.c
   )

add_library(${BUILD_TARGET} STATIC
            ${PBRLAB_SOURCES})

target_compile_features(${BUILD_TARGET}
                        PRIVATE cxx_std_11)   

# Conditionally add SSE4.1 flags when SIMD is enabled for tinybvh
if(NOT PBRLAB_TINYBVH_NO_SIMD AND NOT PBRLAB_WITH_EMBREE)
  set(TINYBVH_SIMD_FLAGS "-msse4.1")
else()
  set(TINYBVH_SIMD_FLAGS "")
endif()

target_compile_options(${BUILD_TARGET} PRIVATE
                       $<$<CXX_COMPILER_ID:Clang>:${PBRLAB_CLANG_COMPILE_OPTIONS} ${TINYBVH_SIMD_FLAGS}>
                       $<$<CXX_COMPILER_ID:GCC>:-Wall -Wextra ${TINYBVH_SIMD_FLAGS}>
                       $<$<CXX_COMPILER_ID:MSVC>:/W4>)  

target_include_directories(${BUILD_TARGET} PUBLIC ${PROJECT_SOURCE_DIR}/src)

target_include_directories(${BUILD_TARGET} SYSTEM PRIVATE 
                           ${PBRLAB_EXT_INCLUDE_DIRS_PRIVATE})

target_link_libraries(${BUILD_TARGET} PRIVATE
                      ${PBRLAB_EXT_LIBRARIES_PRIVATE})

target_link_libraries(${BUILD_TARGET} PUBLIC
                      ${PBRLAB_EXT_LIBRARIES_PUBLIC}
                      Threads::Threads)

set_target_properties(${BUILD_TARGET} PROPERTIES
  OUTPUT_NAME ${BUILD_TARGET}
  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
