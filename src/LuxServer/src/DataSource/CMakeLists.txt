# if the platform is windows, use ADS, otherwise use sim
if(WIN32)
add_subdirectory(ADS)
else()
add_subdirectory(sim)
endif()