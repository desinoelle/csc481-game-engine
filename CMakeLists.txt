cmake_minimum_required(VERSION 3.16)
project(coolEngine)

set(CMAKE_CXX_STANDARD 17)

if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH /opt/homebrew /usr/local)
endif()

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

target_link_libraries(coolEngine PRIVATE SDL3::SDL3 SDL3_image::SDL3_image cppzmq Threads::Threads)

add_executable(coolServer
    src/server_main.cpp
    include/coolMath.cpp
    include/evilNetworking.cpp
)

target_include_directories(coolServer PRIVATE include)

target_link_libraries(coolServer PRIVATE SDL3::SDL3 cppzmq Threads::Threads)

# only build personal games when the file is present
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/monica_main.cpp)
    add_executable(monicaGame
        src/monica_main.cpp
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

    target_include_directories(monicaGame PRIVATE include)

    target_link_libraries(monicaGame PRIVATE SDL3::SDL3 SDL3_image::SDL3_image cppzmq Threads::Threads)
endif()

# ---- Coin Rush (Desiree's individual game) ----

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/desiree_rush_main.cpp)
    add_executable(rushGame
        src/desiree_rush_main.cpp
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
    target_include_directories(rushGame PRIVATE include)
    target_link_libraries(rushGame PRIVATE SDL3::SDL3 SDL3_image::SDL3_image cppzmq Threads::Threads)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/desiree_rush_server.cpp)
    add_executable(rushServer
        src/desiree_rush_server.cpp
        include/coolMath.cpp
        include/evilNetworking.cpp
    )
    target_include_directories(rushServer PRIVATE include)
    target_link_libraries(rushServer PRIVATE cppzmq Threads::Threads)
endif()