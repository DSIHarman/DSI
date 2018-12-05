CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

INCLUDE_DIRECTORIES(. ../../src/base)

SET(CMAKE_CXX_FLAGS -Wno-missing-field-initializers)

CONFIGURE_FILE(testdriver.sh testdriver.sh COPYONLY)

if(CPPUNIT_LIBRARY)
   ADD_EXECUTABLE(test_pingpong CPingPongTest.cpp CDestructionTest.cpp)   
   TARGET_LINK_LIBRARIES(test_pingpong PingPongTest dsi_base dsi_common dsi_servicebroker cppunit testmain rt pthread)
   ADD_TEST(NAME pingpong COMMAND testdriver.sh test_pingpong)
   
   ADD_EXECUTABLE(test_tcp_registration CTCPRegistrationTest.cpp)   
   TARGET_LINK_LIBRARIES(test_tcp_registration PingPongTest dsi_base dsi_common dsi_servicebroker cppunit testmain rt pthread)
   ADD_TEST(NAME tcp_registration COMMAND testdriver.sh test_tcp_registration)
   
   ADD_EXECUTABLE(test_multiple_responses CMultipleResponsesTest.cpp)   
   TARGET_LINK_LIBRARIES(test_multiple_responses PingPongTest dsi_base dsi_common dsi_servicebroker cppunit testmain rt pthread)
   ADD_TEST(NAME multiple_responses COMMAND testdriver.sh test_multiple_responses)
   
   ADD_EXECUTABLE(test_unblock CUnblockTest.cpp)   
   TARGET_LINK_LIBRARIES(test_unblock PingPongTest dsi_base dsi_common dsi_servicebroker cppunit testmain rt pthread)
   ADD_TEST(NAME unblock COMMAND testdriver.sh test_unblock)
   
   ADD_EXECUTABLE(test_attributes CAttributesTest.cpp)   
   TARGET_LINK_LIBRARIES(test_attributes AttributesTest dsi_base dsi_common dsi_servicebroker cppunit testmain rt pthread)
   ADD_TEST(NAME attributes COMMAND testdriver.sh test_attributes)
   
   ADD_EXECUTABLE(test_partial CPartialAttributeUpdateTest.cpp)   
   TARGET_LINK_LIBRARIES(test_partial AttributesTest dsi_base dsi_common dsi_servicebroker cppunit testmain rt pthread)
   ADD_TEST(NAME partial COMMAND testdriver.sh test_partial)
   
   ADD_EXECUTABLE(test_errorenum CErrorEnumTest.cpp)   
   TARGET_LINK_LIBRARIES(test_errorenum ErrorEnumTest dsi_base dsi_common dsi_servicebroker cppunit testmain rt pthread)
   ADD_TEST(NAME errorenum COMMAND testdriver.sh test_errorenum)
   
   ADD_EXECUTABLE(test_register CRegisterTest.cpp)   
   TARGET_LINK_LIBRARIES(test_register RegisterTest dsi_base dsi_common dsi_servicebroker cppunit testmain rt pthread)
   ADD_TEST(NAME register COMMAND testdriver.sh test_register)
endif(CPPUNIT_LIBRARY)

DSI2_GENERATE(PingPongTest.hbsi)
DSI2_GENERATE(AttributesTest.hbsi)
DSI2_GENERATE(ErrorEnumTest.hbsi)
DSI2_GENERATE(RegisterTest.hbsi)
