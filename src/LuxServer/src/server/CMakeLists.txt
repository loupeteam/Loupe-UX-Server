file(GLOB path_SRC "*.cpp" )

#add the library
add_executable(luxserver ${path_SRC} )

#include the local files
target_include_directories(luxserver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#find and link to asio
if(WIN32)
    find_package(asio CONFIG)
endif()

if(WIN32)
    target_link_libraries(luxserver PRIVATE asio::asio)
endif()
target_link_libraries(luxserver PUBLIC jsonserver)
target_link_libraries(luxserver PUBLIC adsdatasrc)
target_link_libraries(luxserver PUBLIC luxutil)

#add the router example
######################
add_executable(server_router example/server.cpp )
#include the local files
target_include_directories(server_router PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
    target_link_libraries(server_router PRIVATE asio::asio)
endif()
target_link_libraries(server_router PUBLIC jsonserver)
target_link_libraries(server_router PUBLIC adsdatasrc)
target_link_libraries(server_router PUBLIC luxutil)

#install the executable
if(WIN32)
install(TARGETS luxserver DESTINATION /)
install(FILES ${CMAKE_SOURCE_DIR}/configuration.json DESTINATION /)
else()
install(TARGETS luxserver DESTINATION /usr/local/bin)
install(FILES ${CMAKE_SOURCE_DIR}/configuration.json DESTINATION /etc/lux/)
endif()