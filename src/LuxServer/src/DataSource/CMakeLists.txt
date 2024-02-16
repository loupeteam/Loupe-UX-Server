# if the platform is windows, use ADS, otherwise use sim
add_subdirectory(ADS)

# Install the header files
file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/lux)