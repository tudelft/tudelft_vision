cmake_minimum_required (VERSION 2.6)
project (vision CXX)

file(GLOB SRCS "cam/*.cpp" "encoding/*.cpp" "vision/*.cpp" "drivers/*.cpp" "targets/*.cpp")
file(GLOB HDRS "cam/*.h" "encoding/*.h" "vision/*.h" "drivers/*.h" "targets/*.h")
include_directories(./)

add_library(${PROJECT_NAME} SHARED ${SRCS})
add_library(${PROJECT_NAME}_static STATIC ${SRCS})
add_custom_target(${PROJECT_NAME}_headers SOURCES ${HDRS})

# Set release as default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

# Style formatting
find_program(ASTYLE_EXECUTABLE astyle DOC "A tool to format the code")
add_custom_target(${PROJECT_NAME}_format ${ASTYLE_EXECUTABLE} --style=google ${SRCS} ${HDRS})

# Lib JPEG
find_package(JPEG REQUIRED)
include_directories(${JPEG_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${JPEG_LIBRARIES})
target_link_libraries(${PROJECT_NAME}_static ${JPEG_LIBRARIES})

# Lib thread
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(${PROJECT_NAME}_static ${CMAKE_THREAD_LIBS_INIT})

# Enable C++11
if (CMAKE_VERSION VERSION_LESS "3.1")
    check_cxx_compiler_flag("-std=c++11" COMPILER_SUPPORTS_CXX11)
    check_cxx_compiler_flag("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
    if(COMPILER_SUPPORTS_CXX11)
        set(CMAKE_CXX_FLAGS "--std=c++11 ${CMAKE_CXX_FLAGS}")
    elseif(COMPILER_SUPPORTS_CXX0X)
        set(CMAKE_CXX_FLAGS "--std=c++0x ${CMAKE_CXX_FLAGS}")
    endif ()
else ()
    set(CMAKE_CXX_STANDARD 11)
endif ()

# Enable warnings
if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_GNUC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
endif ()

# Enable timing output for gprof
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pg -DCLOGGER_ENABLE")
set(CMAKE_CPP_FLAGS_DEBUG "${CMAKE_CPP_FLAGS_DEBUG} -pg -DCLOGGER_ENABLE")

