cmake_minimum_required (VERSION 2.6)

# Decide which target are we building for
if (TARGET)
    if((${TARGET} STREQUAL "bebop") OR (${TARGET} STREQUAL "linux"))
        message ("\nBuilding for ${TARGET}\n")
    else()
        MESSAGE(FATAL_ERROR "Target ${TARGET} is not valid. Choices are \"bebop\" and \"linux\".")
    endif()
else(TARGET)
    message ("\nTARGET was not set. Choices are \"bebop\" and \"linux\".")
    message ("Example usage: cmake .. -DTARGET=linux")
    message ("Now using [linux] as default.\n")
    set (TARGET "linux")
endif(TARGET)

if(${TARGET} STREQUAL "bebop")
    set (CMAKE_TOOLCHAIN_FILE bebop-toolchain.cmake)
endif()

# Set the output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Enable C++11
set(CMAKE_CXX_FLAGS "-std=c++0x")

# Add the vision library
add_subdirectory(../lib tuv_lib)
include_directories(../lib)

# Add all modules
add_subdirectory(video_streamer)
