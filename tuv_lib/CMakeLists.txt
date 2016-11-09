cmake_minimum_required (VERSION 2.6)
project (tuv CXX)

# Include common settings
include(common.cmake)

# List all cpp files
file(GLOB SRCS "src/cam/*.cpp" "src/encoding/*.cpp" "src/vision/*.cpp" "src/drivers/*.cpp" "src/targets/*.cpp")

# List all headers for the main headers
file(REMOVE "include/tuv/tuv.h")
file(GLOB_RECURSE HDRS_INC RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/include"  "include/*.h")

# Generate main header
foreach(filename ${HDRS_INC})
    file(APPEND "include/tuv/tuv.h" "#include <${filename}>\r\n")
endforeach(filename)

# Add headers target for Qt
file(GLOB_RECURSE HDRS "include/*.h")
add_custom_target(${PROJECT_NAME}_headers SOURCES ${HDRS})

# Add library target
add_library(${PROJECT_NAME} ${SRCS})

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
install(TARGETS ${PROJECT_NAME} EXPORT MyLibraryConfig
    ARCHIVE  DESTINATION lib
    LIBRARY  DESTINATION lib
    RUNTIME  DESTINATION bin)
install(DIRECTORY include/ DESTINATION include)

# Style formatting
find_program(ASTYLE_EXECUTABLE astyle DOC "A tool to format the code")
add_custom_target(${PROJECT_NAME}_format ${ASTYLE_EXECUTABLE} --style=google ${SRCS} ${HDRS})

# Lib JPEG
find_package(JPEG REQUIRED)
include_directories(${JPEG_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${JPEG_LIBRARIES})

# Lib thread
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})

# Enable logger output
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DCLOGGER_ENABLE")
set(CMAKE_CPP_FLAGS_DEBUG "${CMAKE_CPP_FLAGS_DEBUG} -DCLOGGER_ENABLE")

