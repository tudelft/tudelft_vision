cmake_minimum_required (VERSION 2.6)
project (video_streamer CXX)

add_executable(${PROJECT_NAME} video_streamer.cxx)
target_link_libraries(${PROJECT_NAME} vision_static)

add_custom_target(upload
 COMMAND curl --upload-file ../bin/video_streamer ftp://192.168.42.1/internal_000/paparazzi/)
