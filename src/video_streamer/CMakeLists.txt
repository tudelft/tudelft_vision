cmake_minimum_required (VERSION 2.6)
project (video_streamer CXX)

set(SUPPORTED_PLATFORMS "Linux" "Bebop")

find_package(JPEG)
if(JPEG_FOUND)
    add_definitions(-DINCLUDE_JPEG=1)
endif()

if (";${SUPPORTED_PLATFORMS};" MATCHES ";${PLATFORM};")
    add_executable(${PROJECT_NAME} video_streamer.cxx)
    add_custom_target(${PROJECT_NAME}_headers SOURCES ${PLATFORM_CONFIG})
    target_link_libraries(${PROJECT_NAME} tuv)
endif ()
