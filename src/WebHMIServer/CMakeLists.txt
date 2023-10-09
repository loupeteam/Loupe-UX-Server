# version 3.11 or later of CMake or needed later for installing GoogleTest
# so let's require it now.
cmake_minimum_required(VERSION 3.11-3.18)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_TOOLCHAIN_FILE "C:/Users/joshpolansky/Documents/clients/loupe/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

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


project(webhmiserver)

add_subdirectory(src)

include_directories(PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

