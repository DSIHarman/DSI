CMAKE_MINIMUM_REQUIRED(VERSION 2.6)


ADD_EXECUTABLE(client client.cpp)
TARGET_LINK_LIBRARIES(client DSIBenchmark dsi_base dsi_common dsi_servicebroker rt pthread)

ADD_EXECUTABLE(server server.cpp)
TARGET_LINK_LIBRARIES(server DSIBenchmark dsi_base dsi_common dsi_servicebroker rt pthread)

DSI2_GENERATE(DSIBenchmark.hbsi)



