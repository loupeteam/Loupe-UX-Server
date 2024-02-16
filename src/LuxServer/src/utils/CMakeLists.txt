file(GLOB path_SRC "*.cpp" )

#add the library
add_library( luxutil STATIC ${path_SRC} )
add_library( luxutil::luxutil ALIAS luxutil )

#include the local files
target_include_directories(luxutil PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include/lux>)

# export library (either static or shared depending on BUILD_SHARED_LIBS)
install(TARGETS luxutil
  EXPORT luxutil
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
  )

file(GLOB HEADERS "util.h")
install(FILES ${HEADERS} DESTINATION include/lux/)

