cmake_minimum_required(VERSION 2.8.12.1)

set(CMAKE_CONFIGURATION_TYPES Debug Release)

set(PROJECT_NAME "d3d-9211")

project(${PROJECT_NAME})

set_property(GLOBAL PROPERTY OS_FOLDERS ON)

if(MSVC)
	add_definitions(-DUNICODE -D_UNICODE)
endif()

set(
    CMAKE_RUNTIME_OUTPUT_DIRECTORY
    ${CMAKE_HOME_DIRECTORY}/bin
    )
	
set(
    CMAKE_LIBRARY_OUTPUT_DIRECTORY
    ${CMAKE_HOME_DIRECTORY}/lib
    )
	
set(
    TARGET_OUTPUT_DIRECTORY
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIGURATION>
    )

add_subdirectory(src)
