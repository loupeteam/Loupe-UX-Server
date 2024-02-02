file(GLOB path_SRC "*.cpp" )

#add the library
add_library( jsonserver ${path_SRC} )

target_link_libraries(jsonserver PUBLIC util)

#find and link to asio
find_package(asio CONFIG)
target_link_libraries(jsonserver PRIVATE asio::asio)

#include the local files
target_include_directories(jsonserver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#include the global include directory for all targets
target_include_directories(jsonserver PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_include_directories(jsonserver PUBLIC ${CMAKE_SOURCE_DIR}/depends/Crow/include)