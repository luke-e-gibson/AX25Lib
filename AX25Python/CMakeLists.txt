cmake_minimum_required(VERSION 3.14)
project(AX25Python LANGUAGES CXX)

set(PYBIND11_FINDPYTHON ON)
find_package(pybind11 CONFIG QUIET)
if(NOT pybind11_FOUND)
	message(STATUS "pybind11 not found; fetching with FetchContent")
	include(FetchContent)
	FetchContent_Declare(
		pybind11
		URL https://github.com/pybind/pybind11/archive/refs/tags/v2.13.6.zip
		DOWNLOAD_EXTRACT_TIMESTAMP ON
	)
	FetchContent_MakeAvailable(pybind11)
endif()

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.hpp")

if(SOURCES STREQUAL "")
	message(WARNING "AX25Python: no sources found under src/ — skipping pybind11 module")
	return()
endif()

set(CMAKE_CXX_STANDARD 17)
     
pybind11_add_module(AX25Python ${SOURCES} ${HEADERS})
target_link_libraries(AX25Python PRIVATE AX25Lib)
install(TARGETS AX25Python
	LIBRARY DESTINATION .
	RUNTIME DESTINATION .
	ARCHIVE DESTINATION .
)

set(AX25PYTHON_PYI ${CMAKE_CURRENT_LIST_DIR}/AX25Python.pyi)
if(EXISTS ${AX25PYTHON_PYI})
	install(FILES ${AX25PYTHON_PYI} DESTINATION .)

	# Copy the stub next to the built .pyd/.so for in-tree development and LSPs.
	add_custom_command(TARGET AX25Python POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		${AX25PYTHON_PYI}
		$<TARGET_FILE_DIR:AX25Python>/AX25Python.pyi
	)
endif()
