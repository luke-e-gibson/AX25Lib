cmake_minimum_required(VERSION 3.14)

project(AX25Console)

include(FetchContent)
FetchContent_Declare(
	inifile-cpp
	GIT_REPOSITORY https://github.com/Rookfighter/inifile-cpp.git
	GIT_TAG main
)
FetchContent_MakeAvailable(inifile-cpp)
ax25lib_assign_folder_recursive("${inifile-cpp_BINARY_DIR}" "vendor")

# Create an interface target for inifile-cpp (header-only)
if(NOT TARGET inifile-cpp)
	add_library(inifile-cpp INTERFACE)
	target_include_directories(inifile-cpp INTERFACE ${inifile-cpp_SOURCE_DIR}/include)
endif()

if(TARGET inifile-cpp)
	set_property(TARGET inifile-cpp PROPERTY FOLDER "vendor")
endif()

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.hpp")
file(GLOB_RECURSE OTHER_FILES "*.ini")

set(CMAKE_CXX_STANDARD 17)


add_executable(AX25Console ${SOURCES} ${HEADERS} ${OTHER_FILES})
target_link_libraries(AX25Console PRIVATE AX25Lib inifile-cpp)

#Copy config.ini to build directory for in-tree development.
add_custom_command(
    TARGET AX25Console POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/config.ini
    $<TARGET_FILE_DIR:AX25Console>/config.ini
)