cmake_minimum_required (VERSION 2.8.12 FATAL_ERROR)

# Get all projects
file(GLOB PROJECTS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/src" "src/*")

# Set output paths
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib") 
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# Link directory for libh1enc.so
link_directories("${CMAKE_CURRENT_SOURCE_DIR}/vendor/lib")

# Add the vision library
add_subdirectory(tuv_lib)
include(tuv_lib/common.cmake)
MESSAGE( STATUS "Compiling for platform:         " ${PLATFORM} )
MESSAGE( STATUS "Build type:                     " ${CMAKE_BUILD_TYPE} )

# Find OpenCV 2.4.13 (optional)
find_package(OpenCV 2.4.13 QUIET)

# Add the projects
foreach(project ${PROJECTS})
    MESSAGE( STATUS "Found project:                  " ${project} )
    add_subdirectory(src/${project})
endforeach(project)

if (PLATFORM STREQUAL Bebop)
    add_custom_target(upload
     COMMAND adb push ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} /data/ftp/internal_000/paparazzi/)
    add_custom_target(connect
     COMMAND adb connect 192.168.42.1:9050)
endif ()
