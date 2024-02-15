file(GLOB path_SRC "*.cpp" )

#find and link to asio
find_package(asio CONFIG)

#add the library
add_executable(server ${path_SRC} )

#include the local files
target_include_directories(server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(server PRIVATE asio::asio)
target_link_libraries(server PUBLIC jsonserver)
target_link_libraries(server PUBLIC adsdatasrc)
target_link_libraries(server PUBLIC util)

#add the router example
######################
add_executable(server_router example/server.cpp )
#include the local files
target_include_directories(server_router PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#link to jsonserver
target_link_libraries(server_router PUBLIC jsonserver)
target_link_libraries(server_router PUBLIC adsdatasrc)
target_link_libraries(server_router PUBLIC util)
target_link_libraries(server_router PRIVATE asio::asio)
