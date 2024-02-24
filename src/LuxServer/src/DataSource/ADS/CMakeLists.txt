file(GLOB path_SRC "*.cpp" )

#add the library
add_library( adsdatasrc STATIC ${path_SRC} )
add_library( adsdatasrc::adsdatasrc ALIAS adsdatasrc )

#include the local files
target_include_directories(adsdatasrc PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include/lux>)

target_link_libraries(adsdatasrc PUBLIC luxutil::luxutil)
target_link_libraries(adsdatasrc PUBLIC ads::ads)

# export library (either static or shared depending on BUILD_SHARED_LIBS)
install(TARGETS adsdatasrc
  EXPORT adsdatasrc_target
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
  )

file(GLOB HEADERS "DataSource.h" "AdsRpc.h")
install(FILES ${HEADERS} DESTINATION include/lux/ads)


