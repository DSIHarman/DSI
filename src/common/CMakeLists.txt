CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

INCLUDE_DIRECTORIES(.)

ADD_LIBRARY(dsi_common STATIC Trigger.cpp   CHandler.cpp CDispatcher.cpp CDevices.cpp CTimer.cpp io.cpp)

INSTALL(TARGETS dsi_common ARCHIVE DESTINATION lib)
