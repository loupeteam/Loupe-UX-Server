file(GLOB path_SRC "*.cpp" )

#add the library
add_library( util ${path_SRC} )

#include the local files
target_include_directories(util PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#include the global include directory for all targets
target_include_directories(util PUBLIC ${CMAKE_SOURCE_DIR}/include)