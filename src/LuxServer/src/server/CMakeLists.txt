file(GLOB path_SRC "*.cpp" )

#add the library
add_executable(server ${path_SRC} )

#find and link to asio
find_package(asio CONFIG)
target_link_libraries(server PRIVATE asio::asio)

#include the local files
target_include_directories(server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#link to jsonserver
target_link_libraries(server PUBLIC jsonserver)
target_link_libraries(server PUBLIC adsdatasrc)
target_link_libraries(server PUBLIC util)