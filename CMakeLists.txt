cmake_minimum_required(VERSION 3.16)
project(coolEngine)


set(CMAKE_CXX_STANDARD 17)

find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)

# Create your game executable target as usual
add_executable(coolEngine WIN32 src/main.cpp)

# Link to the actual SDL3 library.
target_link_libraries(coolEngine PRIVATE SDL3::SDL3)

target_include_directories(coolEngine PRIVATE C:/msys64/mingw64/include)