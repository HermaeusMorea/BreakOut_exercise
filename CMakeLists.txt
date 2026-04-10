cmake_minimum_required(VERSION 3.20)

project(BreakOut LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

set(BREAKOUT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/breakout)

add_executable(breakout
    breakout/src/app/main.cpp
    breakout/src/assets/asset_manager.cpp
    breakout/src/assets/asset_paths.cpp
    breakout/src/core/ball_object.cpp
    breakout/src/core/game.cpp
    breakout/src/core/game_level.cpp
    breakout/src/core/game_object.cpp
    breakout/src/rendering/particle_generator.cpp
    breakout/src/rendering/post_processor.cpp
    breakout/src/rendering/shader.cpp
    breakout/src/rendering/sprite_renderer.cpp
    breakout/src/rendering/texture.cpp
    breakout/src/third_party/glad.c
    breakout/src/third_party/stb_image.cpp
)

target_include_directories(breakout PRIVATE
    ${BREAKOUT_ROOT}/include
    ${BREAKOUT_ROOT}/Dependencies/includes
)

target_compile_definitions(breakout PRIVATE
    BREAKOUT_ASSET_DIR="${BREAKOUT_ROOT}/assets"
)

target_link_libraries(breakout PRIVATE glfw OpenGL::GL)

if(MSVC)
    target_compile_options(breakout PRIVATE /W3 /permissive-)
else()
    target_compile_options(breakout PRIVATE -Wall -Wextra -Wpedantic)
endif()
