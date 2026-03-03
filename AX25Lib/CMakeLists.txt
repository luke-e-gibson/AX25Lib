cmake_minimum_required(VERSION 3.10)

project(AX25Lib)

# Build all targets in this directory with -fPIC so the static library can link into shared modules.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.hpp")

set(CMAKE_CXX_STANDARD 17)

add_library(AX25Lib STATIC ${SOURCES} ${HEADERS})
target_include_directories(AX25Lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)