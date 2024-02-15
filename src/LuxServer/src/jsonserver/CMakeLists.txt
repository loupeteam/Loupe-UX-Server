file(GLOB path_SRC "*.cpp" )

#add the library
add_library( jsonserver ${path_SRC} )

target_link_libraries(jsonserver PUBLIC util)

#find and link to asio
if(WIN32)
    find_package(asio CONFIG)
    target_link_libraries(jsonserver PRIVATE asio::asio)
endif()

#include the local files
target_include_directories(jsonserver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
