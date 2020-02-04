set(PBRLAB_SOURCES
    render.cc
    tinyexr.cc
    tiny_obj_loader.cc
    io/image-io.cc
    io/stb_image.cc
    io/stb_image_write.cc
    mesh/triangle-mesh.cc
   )

add_library(${BUILD_TARGET} SHARED
            ${PBRLAB_SOURCES})

set(PBRLAB_DEP_SOURCES
  ${PROJECT_SOURCE_DIR}/third_party/StaticJSON/src/staticjson.cpp
  )
# set(PBRLAB_DEP_SOURCES
#     ${PROJECT_SOURCE_DIR}/src/trackball.cc
#     ${PROJECT_SOURCE_DIR}/deps/glad/src/glad.c
#     ${PROJECT_SOURCE_DIR}/deps/imgui/imgui.cpp
#     ${PROJECT_SOURCE_DIR}/deps/imgui/imgui_draw.cpp
#     ${PROJECT_SOURCE_DIR}/deps/imgui/imgui_widgets.cpp
#     ${PROJECT_SOURCE_DIR}/deps/imgui/imgui_impl_opengl3.cpp
#     ${PROJECT_SOURCE_DIR}/deps/imgui/imgui_impl_glfw.cpp
#     )


target_compile_features(${BUILD_TARGET}
                        PRIVATE cxx_std_11)   

target_compile_options(${PROJECT_NAME} PRIVATE
                       $<$<CXX_COMPILER_ID:Clang>:${PBRLAB_CLANG_COMPILE_OPTIONS}>
                       $<$<CXX_COMPILER_ID:GCC>:-Wall -Wextra>
                       $<$<CXX_COMPILER_ID:MSVC>:/W4>)  

target_include_directories(${BUILD_TARGET} PUBLIC ${PROJECT_SOURCE_DIR}/src)

# [StaticJson,rapidjson,toml11]
target_include_directories(${BUILD_TARGET} SYSTEM PRIVATE 
                           ${PROJECT_SOURCE_DIR}/third_party/StaticJSON/include
                           ${PROJECT_SOURCE_DIR}/third_party/rapidjson/include
                           ${PROJECT_SOURCE_DIR}/third_party/toml11)

target_link_libraries(${BUILD_TARGET} PRIVATE
                      ${PBRLAB_EXT_LIBRARIES} glfw ${OPENGL_LIBRARIES} ${CMAKE_DL_LIBS})

set_target_properties(${BUILD_TARGET} PROPERTIES
  OUTPUT_NAME ${BUILD_TARGET}
  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
