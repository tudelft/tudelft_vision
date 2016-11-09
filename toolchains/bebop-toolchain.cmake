set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Specify the cross compiler
SET(CMAKE_C_COMPILER   arm-none-linux-gnueabi-gcc)
SET(CMAKE_CXX_COMPILER arm-none-linux-gnueabi-g++)

# where is the target environment
SET(CMAKE_FIND_ROOT_PATH  /opt/arm-2012.03/arm-none-linux-gnueabi/libc)

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
