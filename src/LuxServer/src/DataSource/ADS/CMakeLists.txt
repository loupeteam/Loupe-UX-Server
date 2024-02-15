file(GLOB path_SRC "*.cpp" )

#add the library
add_library( adsdatasrc ${path_SRC} )

target_link_libraries(adsdatasrc PUBLIC util)

#include the local files
target_include_directories(adsdatasrc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(adsdatasrc PUBLIC ads::ads)
