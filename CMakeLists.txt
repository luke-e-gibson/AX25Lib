cmake_minimum_required(VERSION 3.14)
project(AX25Lib LANGUAGES CXX)

option(AX25LIB_BUILD_CONSOLE "Build the AX25 console executable" ON)
option(AX25LIB_BUILD_PYTHON "Build the AX25 Python extension module" ON)

add_subdirectory(AX25Lib)

if(AX25LIB_BUILD_CONSOLE)
	add_subdirectory(AX25Console)
endif()

if(AX25LIB_BUILD_PYTHON)
	add_subdirectory(AX25Python)
endif()