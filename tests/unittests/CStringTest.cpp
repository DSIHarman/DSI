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


class CStringTest : public CppUnit::TestFixture
{
public:
   CPPUNIT_TEST_SUITE(CStringTest);
      CPPUNIT_TEST(testEmpty);
      CPPUNIT_TEST(testFilled);      
   CPPUNIT_TEST_SUITE_END();

public:
   void testEmpty();   
   void testFilled();      
};

CPPUNIT_TEST_SUITE_REGISTRATION(CStringTest);


// --------------------------------------------------------------------------------


void CStringTest::testEmpty()
{
   std::wstring orig(L"");
   
   DSI::CRequestWriter writer;
   DSI::COStream os(writer);                
   os << orig;
   
   std::wstring copy;
   DSI::CIStream is(writer.gptr(), writer.size());
   is >> copy;
   
   CPPUNIT_ASSERT(is.getError() == 0);   
   CPPUNIT_ASSERT(orig == copy);
}


void CStringTest::testFilled()
{
   std::wstring orig(L"Hallo Welt, dies ist ein Text");   
   
   DSI::CRequestWriter writer;
   DSI::COStream os(writer);                
   os << orig;
   
   std::wstring copy;
   DSI::CIStream is(writer.gptr(), writer.size());
   is >> copy;
   
   CPPUNIT_ASSERT(is.getError() == 0);   
   CPPUNIT_ASSERT(orig == copy);
}
