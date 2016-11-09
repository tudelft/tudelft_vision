# At LEAST 2.8 but newer is better
cmake_minimum_required(VERSION 2.8)

# Get all projects
file(GLOB PROJECTS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/src" "src/*")
MESSAGE( STATUS "Projects:         " ${PROJECTS} )

# Set output paths
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib") 
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# Add the vision library
add_subdirectory(tuv_lib)
include(tuv_lib/common.cmake)

# Add the projects
foreach(project ${PROJECTS})
	add_subdirectory(src/${project})
endforeach(project)
