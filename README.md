# CSC 481/581 Game Engine

## Requirements
- CMake 3.16+ and a C++17 compiler
- Ninja (optional)
- cppzmq / libzmq (not bundled; install it first)
  - MSYS2: `pacman -S mingw-w64-x86_64-cppzmq`
  - macOS: `brew install cppzmq`
  - Ubuntu: `sudo apt install libzmq3-dev cppzmq-dev`
- SDL3 and SDL3_image are bundled in `vendored/` and build automatically
  (the first build takes a few minutes).

## Build
    cmake -S . -B build
    cmake --build build

## Run
Peer-to-peer (lockstep): open two terminals.
    ./build/coolEngine 1
    ./build/coolEngine 2
Each instance waits up to 30 seconds for the other peer, so the window
looks frozen until both are running.

Client-server (up to 4 clients):
    ./build/coolServer
    ./build/coolEngine client [host]
