CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

ADD_EXECUTABLE(client_step3 client.cpp)
TARGET_LINK_LIBRARIES(client_step3 Calculator dsi_base dsi_common dsi_servicebroker rt pthread)

ADD_EXECUTABLE(server_step3 server.cpp)
TARGET_LINK_LIBRARIES(server_step3 Calculator dsi_base dsi_common dsi_servicebroker rt pthread)





