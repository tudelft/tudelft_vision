cmake_minimum_required (VERSION 2.6)
project (tuv CXX)

# Include common settings
include(common.cmake)

# List all cpp files
file(GLOB SRCS
    "src/cam/cam.cpp"
    "src/drivers/clogger.cpp"
    "src/targets/target.cpp"
    "src/vision/image.cpp"
    "src/vision/image_buffer.cpp")
file(GLOB SRCS_LINUX
    "src/cam/cam_linux.cpp"
    "src/drivers/i2cbus.cpp"
    "src/drivers/mt9f002.cpp"
    "src/drivers/udpsocket.cpp"
    "src/encoding/encoder_rtp.cpp"
    "src/targets/linux.cpp"
    "src/vision/image_v4l2.cpp")
file(GLOB SRCS_BEBOP
    "src/cam/cam_bebop.cpp"
    "src/drivers/isp.cpp"
    "src/encoding/encoder_h264.cpp")
file(GLOB SRCS_JPEG
    "src/encoding/encoder_jpeg.cpp")

# Platform based sources
if (PLATFORM STREQUAL Linux)
    set(SRCS ${SRCS} ${SRCS_LINUX})
elseif (PLATFORM STREQUAL Bebop)
    set(SRCS ${SRCS} ${SRCS_LINUX} ${SRCS_BEBOP})
endif ()

# Find libjpeg and add sources
find_package(JPEG)
if (JPEG_FOUND)
    include_directories(${JPEG_INCLUDE_DIR})
    set(SRCS ${SRCS} ${SRCS_JPEG})
endif (JPEG_FOUND)

# List all headers for the main headers file
file(REMOVE "include/tuv/tuv.h")
file(GLOB_RECURSE HDRS_INC RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/include"  "include/*.h")

foreach(filename ${HDRS_INC})
    file(APPEND "include/tuv/tuv.h" "#include <${filename}>\r\n")
endforeach(filename)

# Add headers target for Qt
file(GLOB_RECURSE HDRS "include/*.h")
add_custom_target(${PROJECT_NAME}_headers SOURCES ${HDRS})

# Add library target
add_library(${PROJECT_NAME} ${SRCS})
target_link_libraries(${PROJECT_NAME} ${JPEG_LIBRARIES})

# Add all includes
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE src)
target_include_directories(${PROJECT_NAME} PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/tuv>
    $<INSTALL_INTERFACE:include/tuv>
    PRIVATE src)

# Add install command
install(TARGETS ${PROJECT_NAME} EXPORT TUVConfig
    ARCHIVE  DESTINATION lib
    LIBRARY  DESTINATION lib
    RUNTIME  DESTINATION bin)
install(DIRECTORY include/ DESTINATION include)

# Style formatting
find_program(ASTYLE_EXECUTABLE astyle DOC "A tool to format the code")
add_custom_target(${PROJECT_NAME}_format ${ASTYLE_EXECUTABLE} --style=google ${SRCS} ${HDRS})

# Enable logger output
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DCLOGGER_ENABLE")
set(CMAKE_CPP_FLAGS_DEBUG "${CMAKE_CPP_FLAGS_DEBUG} -DCLOGGER_ENABLE")

