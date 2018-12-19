CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

INCLUDE_DIRECTORIES(. ../common)

ADD_LIBRARY(dsi_base STATIC
   CBase.cpp
   CChannel.cpp
   CClient.cpp
   CCommEngine.cpp
   CDummyChannel.cpp
   CIStream.cpp
   COStream.cpp
   CServer.cpp
   CServicebroker.cpp
   DSI.cpp
   Log.cpp
   utf8.cpp
   CRequestReader.cpp
   CBuffer.cpp
   CClientConnectSM.cpp
   CTraceManager.cpp
   util.cpp
   Trace.cpp
   CStdoutTracer.cpp
   CRequestWriter.cpp
)

INSTALL(TARGETS dsi_base ARCHIVE DESTINATION lib)

