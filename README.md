# TU Delft Vision
This is a project which contains several projects and programs using the TUV Library. The aim of this project is to make as much vision programs compatible for multiple platforms. This is done by using the TUV Library which abstracts most of the platform features and capabilities.

## Directory structure
- **src** The source of the different programs. Each directory in this folder is parsed as a project/program.
- **tuv_lib** The TUV Library containing most of the vision functions
- **toolchains** Different CMake toolchain files for the supported platforms.

## How to build
For a simple build on your own platform you could use the following instructions:
- First install the requirements
- Create a new build directory (`mkdir build && cd build`)
- Run cmake (`cmake ../`)

If you want to do more complex options, the following settings for the `cmake` command are available:
- **CMAKE_BUILD_TYPE** With this you can set the build type to enable or disable debugging output. Possible values are *Debug* and *Release*. The default build type is *Release*.
- **CMAKE_TOOLCHAIN_FILE** This file will define the target platform and compiler to use. This is mainly used to cross-compile the library(For example for the bebop). By default CMake compiles for the Host computer and examples of toolchain files can be found in the *toolchains* folder.

## Building requirements
For building the projects you at least need:
- CMake 2.8 (or higher)
- GCC or clang (With c++11 or c++0x support)

For certain programs or platforms you are required to install:
- libjpeg
- linuxgnutools from Parrot (As cross-compiler for the Bebop platform)

## How to build documentation
```
pip install sphinx
pip install recommonmark
cd docs
make html
```