file(GLOB path_SRC "*.cpp" )

#add the library
add_library( adsdatasrc ${path_SRC} )

target_link_libraries(adsdatasrc PUBLIC util)

#include the local files
target_include_directories(adsdatasrc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#if windows use the windows library
if(WIN32)
    target_include_directories(adsdatasrc PUBLIC ${CMAKE_SOURCE_DIR}/depends/ADS/AdsLib)
    target_link_libraries(adsdatasrc PUBLIC debug ${CMAKE_SOURCE_DIR}/lib/debug/ads.lib)
    target_link_libraries(adsdatasrc PUBLIC optimized ${CMAKE_SOURCE_DIR}/lib/release/ads.lib)
endif(WIN32)

#if linux use the linux library
if(UNIX)
    target_link_libraries(adsdatasrc PUBLIC libAdsLib.a)
endif(UNIX)