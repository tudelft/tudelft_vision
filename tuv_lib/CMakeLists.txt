cmake_minimum_required (VERSION 2.8.12 FATAL_ERROR)
project (tuv CXX)

# Include common settings
include(common.cmake)

# List all cpp files
file(GLOB SRCS
    "src/cam/cam.cpp"
    "src/drivers/clogger.cpp"
    "src/targets/target.cpp"
    "src/vision/image.cpp"
    "src/vision/image_buffer.cpp"
    "src/vision/image_ptr.cpp")
file(GLOB SRCS_UNIX
    "src/drivers/udpsocket.cpp"
    "src/encoding/encoder_rtp.cpp")
file(GLOB SRCS_LINUX
    "src/cam/cam_linux.cpp"
    "src/drivers/i2cbus.cpp"
    "src/drivers/mt9f002.cpp"
    "src/drivers/mt9v117.cpp"
    "src/targets/linux.cpp")
file(GLOB SRCS_BEBOP
    "src/cam/cam_bebop_front.cpp"
    "src/cam/cam_bebop_bottom.cpp"
    "src/drivers/isp.cpp"
    "src/encoding/encoder_h264.cpp"
    "src/vision/image_h264.cpp"
    "src/targets/bebop.cpp")
file(GLOB SRCS_JPEG
    "src/encoding/encoder_jpeg.cpp")
set(SRCS_ALL ${SRCS} ${SRCS_UNIX} ${SRCS_LINUX} ${SRCS_BEBOP} ${SRCS_JPEG})

# Platform based sources
if (PLATFORM STREQUAL Linux)
    set(SRCS ${SRCS} ${SRCS_UNIX} ${SRCS_LINUX})
elseif (PLATFORM STREQUAL OSX)
    set(SRCS ${SRCS} ${SRCS_UNIX})
elseif (PLATFORM STREQUAL Bebop)
    set(SRCS ${SRCS} ${SRCS_UNIX} ${SRCS_LINUX} ${SRCS_BEBOP})
    set(LIBS ${LIBS} "h1enc")
endif ()

# Find libjpeg and add sources
find_package(JPEG)
if (JPEG_FOUND)
    include_directories(${JPEG_INCLUDE_DIR})
    set(SRCS ${SRCS} ${SRCS_JPEG})
    set(LIBS ${LIBS} ${JPEG_LIBRARIES})
endif (JPEG_FOUND)

# Find OpenCV 2.4.13
find_package(OpenCV 2.4.13 QUIET)
if (OpenCV_FOUND)
    set(LIBS ${LIBS} ${OpenCV_LIBS})
endif (OpenCV_FOUND)

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
target_link_libraries(${PROJECT_NAME} ${LIBS})

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
add_custom_target(${PROJECT_NAME}_format ${ASTYLE_EXECUTABLE} --style=google ${SRCS_ALL} ${HDRS})

# Enable logger output
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DCLOGGER_ENABLE")
set(CMAKE_CPP_FLAGS_DEBUG "${CMAKE_CPP_FLAGS_DEBUG} -DCLOGGER_ENABLE")

