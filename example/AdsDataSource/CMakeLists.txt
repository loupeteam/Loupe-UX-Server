cmake_minimum_required(VERSION 3.11-3.18)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(adstest)

file(GLOB path_SRC "*.cpp" )


#add the library
add_executable(server ${path_SRC} )

#include the local files
target_include_directories(server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
    find_library(adsdatasrc_LIBRARY_PATH adsdatasrc HINTS "C:/Program Files/luxserver/lib/")
    find_library(ads_LIBRARY_PATH ads HINTS "C:/Program Files/luxserver/lib/")
    find_library(luxutil_LIBRARY_PATH luxutil HINTS "C:/Program Files/luxserver/lib/")
    include_directories( "C:/Program Files/luxserver/include/" )
    include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/include/ )
    link_directories( "C:/Program Files/luxserver/lib/" )
    target_link_libraries(server PUBLIC Ws2_32)
else()
find_library(adsdatasrc_LIBRARY_PATH adsdatasrc)
find_library(ads_LIBRARY_PATH ads )
find_library(luxutil_LIBRARY_PATH luxutil )
endif()


target_link_libraries(server PUBLIC ${adsdatasrc_LIBRARY_PATH})
target_link_libraries(server PUBLIC ${ads_LIBRARY_PATH})
target_link_libraries(server PUBLIC ${luxutil_LIBRARY_PATH})
