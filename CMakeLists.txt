cmake_minimum_required(VERSION 3.16)
project(GraphicsLibrary CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# GL-free core: builds everywhere and is what the native tests link against.
add_library(gfx_core STATIC engine/src/params.cpp engine/src/blackbody.cpp)
target_include_directories(gfx_core PUBLIC engine/include)

if(EMSCRIPTEN)
  # Full engine + simulations, compiled to WebAssembly for WebGL2.
  add_executable(engine
    engine/src/gl.cpp engine/src/postfx.cpp engine/src/engine.cpp
    sims/blackhole/BlackHoleScene.cpp sims/waterfall/WaterfallScene.cpp
    platform/web/main.cpp)
  target_link_libraries(engine PRIVATE gfx_core)
  target_compile_options(engine PRIVATE -Wall -Wextra)
  target_link_options(engine PRIVATE
    -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2
    -sALLOW_MEMORY_GROWTH=1 -sMODULARIZE=1 -sEXPORT_NAME=createEngine
    -sENVIRONMENT=web -sEXPORTED_RUNTIME_METHODS=ccall,cwrap,UTF8ToString
    -sFILESYSTEM=0 --no-entry)
  set_target_properties(engine PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/web)
else()
  add_executable(tests tests/tests.cpp)
  target_link_libraries(tests PRIVATE gfx_core)
  target_compile_options(tests PRIVATE -Wall -Wextra)
  enable_testing()
  add_test(NAME tests COMMAND tests)
endif()
