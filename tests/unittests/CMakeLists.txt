CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

INCLUDE_DIRECTORIES(. ../../src/common ../../src/base)

SET(CMAKE_CXX_FLAGS -Wno-missing-field-initializers)

if(CPPUNIT_LIBRARY)
   ADD_EXECUTABLE(test_unittests TVariantTest.cpp CEnumerationTest.cpp CRangeUpdateTest.cpp CStringTest.cpp CVectorTest.cpp)   
   TARGET_LINK_LIBRARIES(test_unittests dsi_servicebroker dsi_common dsi_base cppunit testmain)
   
   ADD_TEST(unittests test_unittests)
endif(CPPUNIT_LIBRARY)

