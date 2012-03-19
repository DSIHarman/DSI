CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

ADD_EXECUTABLE(ping ping.cpp)
TARGET_LINK_LIBRARIES(ping PingPong dsi_base dsi_common dsi_servicebroker rt pthread)

ADD_EXECUTABLE(pong pong.cpp)
TARGET_LINK_LIBRARIES(pong PingPong dsi_base dsi_common dsi_servicebroker rt pthread)

DSI2_GENERATE(PingPong.hbsi)



