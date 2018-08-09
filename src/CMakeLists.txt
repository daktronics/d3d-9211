
set(ALL_SRCS
	app.rc
	platform.h
	resource.h
)

#indicate the entry point for the executable
add_executable (${PROJECT_NAME} WIN32 ${ALL_SRCS})

source_group("src" FILES ${ALL_SRCS})

# Indicate which libraries to include during the link process.
target_link_libraries (${PROJECT_NAME} d3d11.lib Shlwapi.lib)
