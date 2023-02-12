# Static JSON
set (POSSIBLE_RAPIDJSON_DIR "rapidjson/include")
find_path(RAPIDJSON_INCLUDE_DIR rapidjson/rapidjson.h PATHS ${POSSIBLE_RAPIDJSON_DIR})
if (NOT RAPIDJSON_INCLUDE_DIR)
    message("Rapid JSON was not found")
endif()

set(STATICJSON_SOURCE_FILES StaticJSON/src/staticjson.cpp)
add_library(staticjson STATIC ${STATICJSON_SOURCE_FILES})

target_include_directories(staticjson PUBLIC SYSTEM ${RAPIDJSON_INCLUDE_DIR})
target_include_directories(staticjson PUBLIC StaticJSON/include)

target_compile_options(staticjson PRIVATE
          $<$<CXX_COMPILER_ID:Clang>:  -Wall -Wextra -pedantic -g>
          $<$<CXX_COMPILER_ID:GCC>:    -Wall -Wextra -pedantic -g>
          $<$<CXX_COMPILER_ID:MSVC>:>)
 
target_compile_features(staticjson
                        PRIVATE cxx_std_11)   
