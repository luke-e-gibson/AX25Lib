cmake_minimum_required(VERSION 3.14)
project(AX25Lib LANGUAGES CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

function(ax25lib_assign_folder_recursive directory folder_name)
	if(NOT IS_DIRECTORY "${directory}")
		return()
	endif()

	get_property(directory_targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
	foreach(target_name IN LISTS directory_targets)
		set_property(TARGET ${target_name} PROPERTY FOLDER "${folder_name}")
	endforeach()

	get_property(child_directories DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
	foreach(child_directory IN LISTS child_directories)
		ax25lib_assign_folder_recursive("${child_directory}" "${folder_name}")
	endforeach()
endfunction()

option(AX25LIB_BUILD_CONSOLE "Build the AX25 console executable" ON)
option(AX25LIB_BUILD_PYTHON "Build the AX25 Python extension module" ON)

add_subdirectory(AX25Lib)

if(AX25LIB_BUILD_CONSOLE)
	add_subdirectory(AX25Console)
endif()

if(AX25LIB_BUILD_PYTHON)
	add_subdirectory(AX25Python)
endif()