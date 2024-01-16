file(GLOB path_SRC "*.cpp" )

#add the library
add_library( adsdatasrc ${path_SRC} )

#include the local files
target_include_directories(adsdatasrc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#include the ADS API
target_include_directories(adsdatasrc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../../dependencies/Include)

#include the global include directory for all targets
target_include_directories(adsdatasrc PUBLIC ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(adsdatasrc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../../dependencies/TcAdsDll.lib)

#find and link to asio
find_package(asio CONFIG)
target_link_libraries(adsdatasrc PRIVATE asio::asio)