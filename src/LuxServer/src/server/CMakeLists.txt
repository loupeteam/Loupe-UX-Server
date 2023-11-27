file(GLOB path_SRC "*.cpp" )

#add the library
add_executable(server ${path_SRC} )

#find and link to asio
find_package(asio CONFIG)
target_link_libraries(server PRIVATE asio::asio)

#include the local files
target_include_directories(server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#include the global include directory for all targets
target_include_directories(server PUBLIC ${CMAKE_SOURCE_DIR}/include)

#link to jsonserver
target_link_libraries(server PUBLIC jsonserver)
target_include_directories(server PUBLIC $<TARGET_PROPERTY:jsonserver,INTERFACE_INCLUDE_DIRECTORIES>)

target_link_libraries(server PUBLIC adsdatasrc)
target_link_libraries(server PUBLIC util)