
set(ALL_SRCS
	app.rc
	main.cpp
	platform.h
	resource.h
	util.cpp
	util.h
)

#indicate the entry point for the executable
add_executable (${PROJECT_NAME} WIN32 ${ALL_SRCS})

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")

source_group("src" FILES ${ALL_SRCS})

# Indicate which libraries to include during the link process.
target_link_libraries (${PROJECT_NAME} d3d11.lib Shlwapi.lib)
