cmake_minimum_required (VERSION 2.6)
project (vision CXX)

file(GLOB SRCS "cam/*.cpp" "encoding/*.cpp" "vision/*.cpp" "drivers/*.cpp")
file(GLOB HDRS "cam/*.h" "encoding/*.h" "vision/*.h" "drivers/*.h")
include_directories(./)

add_library(${PROJECT_NAME} SHARED ${SRCS})
add_library(${PROJECT_NAME}_static STATIC ${SRCS})
add_custom_target(${PROJECT_NAME}_headers SOURCES ${HDRS})

# Lib JPEG
find_package(JPEG REQUIRED)
include_directories(${JPEG_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${JPEG_LIBRARIES})
target_link_libraries(${PROJECT_NAME}_static ${JPEG_LIBRARIES})

# Enable C++11
set(CMAKE_CXX_FLAGS "-std=gnu++0x")
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET ${PROJECT_NAME}_static PROPERTY CXX_STANDARD 11)
set_property(TARGET ${PROJECT_NAME}_static PROPERTY CXX_STANDARD_REQUIRED ON)
