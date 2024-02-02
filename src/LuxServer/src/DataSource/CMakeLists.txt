# if the platform is windows, use ADS, otherwise use sim
add_subdirectory(ADS)
if(WIN32)
#add_subdirectory(ADS)
else()
#add_subdirectory(sim)
endif()
