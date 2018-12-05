CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

INCLUDE_DIRECTORIES(. ../../src/common)

SET(CMAKE_CXX_FLAGS -Wno-missing-field-initializers)

if(CPPUNIT_LIBRARY)
   CONFIGURE_FILE(testdriver.sh testdriver.sh)
   CONFIGURE_FILE(servicebroker.cfg servicebroker.cfg)
   
   ADD_EXECUTABLE(test_servicebroker test_servicebroker.cpp)   
   TARGET_LINK_LIBRARIES(test_servicebroker rt dsi_servicebroker cppunit dsi_common testmain)
   
   ADD_TEST(NAME servicebroker COMMAND testdriver.sh)      
endif(CPPUNIT_LIBRARY)

