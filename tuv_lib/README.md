# TUV Library
The TUV library is a multi-platform vision library and abstraction. This Library enables the use of simple image and camera features for different platforms. The library is released under the GPL-3.0 license and can either be included as a static or dynamically loaded library.

## Supported platforms
Currently the following platforms are supported by this library:
- Generic Linux
- Mac OSX (Without webcam/camera support)
- Bebop (Needs an arm-linux cross-compiler)

## How to build
For a simple build on your own platform you could use the following instructions:
- First install the requirements
- Create a new build directory (`mkdir build && cd build`)
- Run cmake (`cmake ../`)

If you want to do more complex options, the following settings for the `cmake` command are available:
- **CMAKE_BUILD_TYPE** With this you can set the build type to enable or disable debugging output. Possible values are *Debug* and *Release*. The default build type is *Release*.
- **CMAKE_TOOLCHAIN_FILE** This file will define the target platform and compiler to use. This is mainly used to cross-compile the library(For example for the bebop). By default CMake compiles for the Host computer.

## Building requirements
For building the TUV Library you at least need the following:
- CMake 2.8 (or higher)
- GCC or clang (With c++11 or c++0x support)

If you want to make use of all the features in the library you're advised to also install the following:
- libjpeg (Enables JPEG encoding)
- linuxgnutools from Parrot (As cross-compiler for the Bebop platform)
