CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

ADD_EXECUTABLE(systemtimer systemtimer.cpp)
TARGET_LINK_LIBRARIES(systemtimer dsi_base dsi_common dsi_servicebroker rt pthread)
