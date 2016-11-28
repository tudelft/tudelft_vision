# Set default platform as host OS
if (NOT DEFINED PLATFORM)
    if (WIN32)
        set(PLATFORM Windows)
    elseif (APPLE)
        set(PLATFORM OSX)
    elseif (UNIX)
        set(PLATFORM Linux)
    endif ()
endif (NOT DEFINED PLATFORM)
add_definitions(-DPLATFORM=${PLATFORM})
add_definitions(-DPLATFORM_${PLATFORM}=true)

# Define the platform configuration file
string(TOLOWER ${PLATFORM} PLATFORM_CONFIG)
set(PLATFORM_CONFIG "${PLATFORM_CONFIG}_config.h")
add_definitions(-DPLATFORM_CONFIG=\"${PLATFORM_CONFIG}\")

# Set release as default build type
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif (NOT CMAKE_BUILD_TYPE)

# Enable C++11
if (CMAKE_VERSION VERSION_LESS "3.1")
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("-std=c++11" COMPILER_SUPPORTS_CXX11)
    check_cxx_compiler_flag("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
    if(COMPILER_SUPPORTS_CXX11)
        set(CMAKE_CXX_FLAGS "--std=c++11 ${CMAKE_CXX_FLAGS}")
    elseif(COMPILER_SUPPORTS_CXX0X)
        set(CMAKE_CXX_FLAGS "--std=c++0x ${CMAKE_CXX_FLAGS}")
    endif ()
else ()
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED on)
endif ()

# Enable warnings
if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_GNUC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
endif ()

# Enable timing output for gprof
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb3 -pg")
set(CMAKE_CPP_FLAGS_DEBUG "${CMAKE_CPP_FLAGS_DEBUG} -ggdb3 -pg")
