CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

ADD_EXECUTABLE(client_step1 client.cpp)
TARGET_LINK_LIBRARIES(client_step1 Calculator dsi_base dsi_common dsi_servicebroker rt pthread)

ADD_EXECUTABLE(server_step1 server.cpp)
TARGET_LINK_LIBRARIES(server_step1 Calculator dsi_base dsi_common dsi_servicebroker rt pthread)





