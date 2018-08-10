
set(ALL_SRCS
	app.rc
	assets.h
	assets.cpp
	d3d.cpp
	d3d.h	
	d3d11.cpp
	d3d11.h	
	main.cpp
	platform.h
	renderer.cpp
	renderer9.cpp
	renderer11.cpp
	resource.h
	scene.h
	util.cpp
	util.h
)

#indicate the entry point for the executable
add_executable (${PROJECT_NAME} WIN32 ${ALL_SRCS})

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")

source_group("src" FILES ${ALL_SRCS})

# Indicate which libraries to include during the link process.
target_link_libraries (${PROJECT_NAME} d3d9.lib d3d11.lib d2d1.lib dwrite.lib Shlwapi.lib)
