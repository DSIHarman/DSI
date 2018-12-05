CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

INCLUDE_DIRECTORIES(. ../common)

ADD_EXECUTABLE(servicebroker    
   ClientList.cpp
   ClientSpecificData.cpp
   ConfigFile.cpp
   ConnectionContext.cpp
   FileLock.cpp
   InterfaceDescription.cpp
   JobQueue.cpp
   Log.cpp
   Main.cpp
   MasterAdapter.cpp
   MessageContext.cpp
   Notification.cpp
   NotificationList.cpp
   NotificationPool.cpp
   Notifier.cpp 
   PulseChannelManager.cpp 
   RegExp.cpp 
   ServerList.cpp 
   Servicebroker.cpp 
   ServicebrokerServer.cpp 
   SocketConnectionContext.cpp 
   SocketMessageContext.cpp 
   TCPMasterNotificationReceiver.cpp 
   util.cpp 
   utilLINUX.cpp 
   WorkerThread.cpp
)

TARGET_LINK_LIBRARIES(servicebroker rt dsi_common)

ADD_EXECUTABLE(sbcat sbcat.cpp)
TARGET_LINK_LIBRARIES(sbcat dsi_common)

ADD_LIBRARY(dsi_servicebroker STATIC clientIo.cpp clientlib.c)

INSTALL(TARGETS servicebroker RUNTIME DESTINATION bin)
INSTALL(TARGETS sbcat RUNTIME DESTINATION bin)

INSTALL(TARGETS dsi_servicebroker ARCHIVE DESTINATION lib)

