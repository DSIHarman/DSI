/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "dsi/COStream.hpp"
#include "dsi/CIStream.hpp"
#include "dsi/DSI.hpp"

#include "CDummyChannel.hpp"


enum MyTestEnum
{
   MyTestEnum_VALUE_0  =  0,
   MyTestEnum_VALUE_5  =  5,
   MyTestEnum_VALUE_10 = 10
};


class CEnumerationTest : public CppUnit::TestFixture
{
public:

   enum MyMemberTestEnum
   {
      MyMemberTestEnum_VALUE_0  =  0,
      MyMemberTestEnum_VALUE_5  =  5,
      MyMemberTestEnum_VALUE_10 = 10
   };

   CPPUNIT_TEST_SUITE(CEnumerationTest);
      CPPUNIT_TEST(testGlobalSerialization);      
      CPPUNIT_TEST(testMemberSerialization);      
   CPPUNIT_TEST_SUITE_END();

public:
   void testGlobalSerialization();      
   void testMemberSerialization();      
};

CPPUNIT_TEST_SUITE_REGISTRATION(CEnumerationTest);


// --------------------------------------------------------------------------------


void CEnumerationTest::testGlobalSerialization()
{       
   MyTestEnum e = MyTestEnum_VALUE_5;
   
   DSI::CRequestWriter writer;
   DSI::COStream os(writer);                
   os << e;
      
   CPPUNIT_ASSERT(writer.size() == sizeof(uint32_t));
   
   MyTestEnum e1;
   DSI::CIStream is(writer.gptr(), writer.size());
   is >> e1;
   
   CPPUNIT_ASSERT(e == e1);   
}


void CEnumerationTest::testMemberSerialization()
{       
   MyMemberTestEnum e = MyMemberTestEnum_VALUE_5;
   
   DSI::CRequestWriter writer;
   DSI::COStream os(writer);                
   os << e;
      
   CPPUNIT_ASSERT(writer.size() == sizeof(uint32_t));
   
   MyMemberTestEnum e1;
   DSI::CIStream is(writer.gptr(), writer.size());
   is >> e1;
   
   CPPUNIT_ASSERT(e == e1);   
}
