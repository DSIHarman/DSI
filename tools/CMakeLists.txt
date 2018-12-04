CMAKE_MINIMUM_REQUIRED(VERSION 2.6)


#
# Generate one hbsi a time - no search paths yet supported.
#
MACRO(DSI2_GENERATE hbsi)

   GET_FILENAME_COMPONENT(basename ${ARGV0} NAME_WE)     
   
   ADD_CUSTOM_COMMAND(
      OUTPUT 
         ${CMAKE_CURRENT_BINARY_DIR}/DSI${basename}Stream.hpp
         ${CMAKE_CURRENT_BINARY_DIR}/DSI${basename}Stream.cpp
         ${CMAKE_CURRENT_BINARY_DIR}/DSI${basename}.hpp
         ${CMAKE_CURRENT_BINARY_DIR}/DSI${basename}.cpp
         ${CMAKE_CURRENT_BINARY_DIR}/C${basename}DSIProxy.hpp
         ${CMAKE_CURRENT_BINARY_DIR}/C${basename}DSIProxy.cpp
         ${CMAKE_CURRENT_BINARY_DIR}/C${basename}DSIStub.hpp
         ${CMAKE_CURRENT_BINARY_DIR}/C${basename}DSIStub.cpp
      COMMAND java ARGS -jar ${CMAKE_HOME_DIRECTORY}/tools/dsi2gen.jar ${CMAKE_CURRENT_SOURCE_DIR}/${ARGV0} ${CMAKE_CURRENT_BINARY_DIR}
      DEPENDS ${CMAKE_HOME_DIRECTORY}/tools/dsi2gen.jar
      MAIN_DEPENDENCY ${ARGV0}
   )

   INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
   ADD_LIBRARY(${basename} STATIC 
      DSI${basename}Stream.cpp 
      DSI${basename}.cpp 
      C${basename}DSIProxy.cpp 
      C${basename}DSIStub.cpp 
   )

ENDMACRO(DSI2_GENERATE)
