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


class CVectorTest : public CppUnit::TestFixture
{
public:
   CPPUNIT_TEST_SUITE(CVectorTest);
      CPPUNIT_TEST(testEmpty);
      CPPUNIT_TEST(testFilled);      
   CPPUNIT_TEST_SUITE_END();

public:
   void testEmpty();
   void testFilled();
};

CPPUNIT_TEST_SUITE_REGISTRATION(CVectorTest);


// --------------------------------------------------------------------------------


void CVectorTest::testEmpty()
{
   std::vector<int> orig;   
   
   DSI::CRequestWriter writer;
   DSI::COStream os(writer);                
   os << orig;
   
   std::vector<int> copy;
   DSI::CIStream is(writer.gptr(), writer.size());
   is >> copy;
   
   CPPUNIT_ASSERT(is.getError() == 0);   
   CPPUNIT_ASSERT(orig == copy);
}


void CVectorTest::testFilled()
{
   std::vector<int> orig;
   orig.push_back(42);
   orig.push_back(43);   
   
   DSI::CRequestWriter writer;
   DSI::COStream os(writer);                
   os << orig;
   
   std::vector<int> copy;
   DSI::CIStream is(writer.gptr(), writer.size());
   is >> copy;
   
   CPPUNIT_ASSERT(is.getError() == 0);   
   CPPUNIT_ASSERT(orig == copy);
}
