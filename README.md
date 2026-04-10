This is a Breakout exercise based on LearnOpenGL.

## What It Is

This project is a small 2D Breakout clone written in C++ with OpenGL.
It uses GLFW for window/input handling, GLAD for OpenGL loading, GLM for math, and stb_image for texture loading.

## Project Layout

```text
breakout/
  assets/
    levels/
    shaders/
    textures/
  include/breakout/
    assets/
    core/
    rendering/
  src/
    app/
    assets/
    core/
    rendering/
    third_party/
```

Notable renames:

- `program.cpp` -> `src/app/main.cpp`
- `resource_manager.*` -> `assets/asset_manager.*`
- runtime assets moved under `breakout/assets/`

## Build Environments

The repository now supports two main build paths:

- Windows with Visual Studio using the existing solution/project files
- Cross-platform builds with CMake on Linux, WSL, and Windows

## Linux / WSL Dependencies

On Ubuntu-based systems, install:

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build pkg-config libglfw3-dev libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
```

For WSL graphics output, use WSLg or another working X11/OpenGL setup.

## Build With CMake

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

Then run:

```bash
./build/breakout
```
