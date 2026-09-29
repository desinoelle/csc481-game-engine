cmake_minimum_required(VERSION 3.16)
project(coolEngine)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(vendored/SDL)

set(SDLIMAGE_VENDORED OFF CACHE BOOL "" FORCE)
add_subdirectory(vendored/SDL_image)

find_package(cppzmq REQUIRED)
find_package(Threads REQUIRED)

add_executable(coolEngine 
    src/main.cpp
    include/input.cpp
    include/entity.cpp
    include/coolMath.cpp
    include/coolPhysics.cpp
    include/sprite.cpp
    include/evilNetworking.cpp
    include/timeMagic.cpp
    include/niceJobSystem.cpp
    include/saucyPeerToPeer.cpp
)

target_include_directories(coolEngine PRIVATE include)

target_link_libraries(coolEngine PRIVATE SDL3::SDL3 SDL3_image::SDL3_image cppzmq)

add_executable(coolServer
    src/server_main.cpp
    include/coolMath.cpp
    include/evilNetworking.cpp
)

target_include_directories(coolServer PRIVATE include)

target_link_libraries(coolServer PRIVATE cppzmq Threads::Threads)
