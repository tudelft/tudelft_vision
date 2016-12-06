cmake_minimum_required (VERSION 2.6)
project (video_streamer CXX)

set(SUPPORTED_PLATFORMS "Linux" "Bebop")

if (";${SUPPORTED_PLATFORMS};" MATCHES ";${PLATFORM};")
    add_executable(${PROJECT_NAME} video_streamer.cxx)
    add_custom_target(${PROJECT_NAME}_headers SOURCES ${PLATFORM_CONFIG})
    target_link_libraries(${PROJECT_NAME} tuv)

    add_custom_target(upload
     COMMAND adb push ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME} /data/ftp/internal_000/paparazzi/
     DEPENDS ${PROJECT_NAME})
endif ()
