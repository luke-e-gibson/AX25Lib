cmake_minimum_required(VERSION 3.10)

project(AX25Lib)

find_package(ZLIB QUIET)
if(NOT ZLIB_FOUND)
	include(FetchContent)
	FetchContent_Declare(
		zlib
		URL https://github.com/madler/zlib/archive/refs/tags/v1.3.1.zip
		DOWNLOAD_EXTRACT_TIMESTAMP ON
	)
	FetchContent_MakeAvailable(zlib)
	ax25lib_assign_folder_recursive("${zlib_BINARY_DIR}" "vendor")
	if(NOT TARGET ZLIB::ZLIB)
		if(TARGET zlibstatic)
			add_library(ZLIB::ZLIB ALIAS zlibstatic)
		elseif(TARGET zlib)
			add_library(ZLIB::ZLIB ALIAS zlib)
		endif()
	endif()
endif()

# Build all targets in this directory with -fPIC so the static library can link into shared modules.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.hpp")

set(CMAKE_CXX_STANDARD 17)

add_library(AX25Lib STATIC ${SOURCES} ${HEADERS})
target_include_directories(AX25Lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(AX25Lib PUBLIC ZLIB::ZLIB)