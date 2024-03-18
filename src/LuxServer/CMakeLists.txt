# version 3.11 or later of CMake or needed later for installing GoogleTest
# so let's require it now.
cmake_minimum_required(VERSION 3.11-3.18)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(WIN32)
     set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake)
endif()

if (CMAKE_GENERATOR MATCHES "Visual Studio")
     add_definitions(-D_USE_MATH_DEFINES)
else()
     add_definitions(-D_USE_MATH_DEFINES)
     add_compile_options(-fpermissive -Wno-write-strings)
     add_compile_options(-Wint-to-pointer-cast)
     add_compile_options(-std=c++17)     
endif()

IF(APPLE)
  SET(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures for Mac OS X" FORCE)
ENDIF(APPLE)


project(luxserver)

# Don't build shared libraries
set(BUILD_SHARED_LIBS OFF)

include_directories(PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# set the install directory
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/install)

add_definitions(-DNOMINMAX)

add_subdirectory(src)
add_subdirectory(depends/ADS)


