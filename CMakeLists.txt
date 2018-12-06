CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

PROJECT(dsi)

SET(CMAKE_VERBOSE_MAKEFILE true)
SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_COMPILER "g++")
SET(CMAKE_POSITION_INDEPENDENT_CODE ON)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=zero-as-null-pointer-constant" )
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=old-style-cast" )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread" )

# enable unittests via 'make test'
ENABLE_TESTING()

# add the include subtree to the header search area
INCLUDE_DIRECTORIES(include)

# compiler directives
ADD_DEFINITIONS(-Wall -W)

# add the DSI2_GENERATE rule
ADD_SUBDIRECTORY(tools)

# add the source directories
ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(examples)
ADD_SUBDIRECTORY(tests)

# install the dsi generator macros
INSTALL(FILES tools/CMakeLists.txt DESTINATION share/dsi)

# install header files
FILE(GLOB DSI_FILES "include/dsi/*.h*")
FILE(GLOB DSI_PVT_FILES "include/dsi/private/*.h*")

INSTALL(FILES ${DSI_FILES} DESTINATION include/dsi)
INSTALL(FILES ${DSI_PVT_FILES} DESTINATION include/dsi/private)

# ubuntu systems only if not cross-compiling
IF(NOT CMAKE_CROSSCOMPILING)
   IF(IS_DIRECTORY /etc/init)  
      # configure upstart script
      CONFIGURE_FILE(tools/servicebroker.conf tools)
   
      INSTALL(FILES ${PROJECT_BINARY_DIR}/tools/servicebroker.conf DESTINATION /etc/init)
   ENDIF()
ENDIF()   
