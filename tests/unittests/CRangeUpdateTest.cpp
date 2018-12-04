/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "dsi/private/attributes.hpp"
#include "dsi/DSI.hpp"


class CRangeUpdateTest : public CppUnit::TestFixture
{
public:
   CPPUNIT_TEST_SUITE(CRangeUpdateTest);
      CPPUNIT_TEST(testCompleteSetter);      
      CPPUNIT_TEST(testInsertSetter);      
      CPPUNIT_TEST(testDeleteSetter);      
      CPPUNIT_TEST(testDeleteSetterRest);      
      CPPUNIT_TEST(testReplaceSetter);      
   CPPUNIT_TEST_SUITE_END();

public:   
   void testCompleteSetter();      
   void testInsertSetter();
   void testDeleteSetter();
   void testDeleteSetterRest();
   void testReplaceSetter();
};

CPPUNIT_TEST_SUITE_REGISTRATION(CRangeUpdateTest);


// --------------------------------------------------------------------------------


void CRangeUpdateTest::testCompleteSetter()
{   
   DSI::Private::ServerAttribute<std::vector<int> > orig;   
   orig.mValue.push_back(1);
   orig.mValue.push_back(2);
   orig.mValue.push_back(3);
   orig.mValue.push_back(4);
   orig.mValue.push_back(5);
   orig.mValue.push_back(6);
   
   std::vector<int> updt;
   updt.push_back(33);
   updt.push_back(34);
   updt.push_back(35);
   
   DSI::UpdateType up = DSI::UPDATE_COMPLETE;
   int16_t pos = 0;
   int16_t count = 0;
   orig.set(updt, up, &pos, &count);
   
   CPPUNIT_ASSERT(orig.mValue == updt);
   
   // check output arguments
   CPPUNIT_ASSERT(pos == 0);
   CPPUNIT_ASSERT(count = updt.size());      
}


void CRangeUpdateTest::testInsertSetter()
{   
   DSI::Private::ServerAttribute<std::vector<int> > orig;
   orig.mValue.push_back(1);
   orig.mValue.push_back(2);
   orig.mValue.push_back(3);
   orig.mValue.push_back(4);
   orig.mValue.push_back(5);
   orig.mValue.push_back(6);
   
   std::vector<int> updt;
   updt.push_back(33);
   updt.push_back(34);
   updt.push_back(35);
   
   DSI::UpdateType up = DSI::UPDATE_INSERT;
   int16_t pos = 3;
   int16_t count = 1;
   orig.set(updt, up, &pos, &count);
   
   std::vector<int> comp;
   comp.push_back(1);
   comp.push_back(2);
   comp.push_back(3);
   comp.push_back(33);
   comp.push_back(34);
   comp.push_back(35);
   comp.push_back(4);
   comp.push_back(5);
   comp.push_back(6);
   
   CPPUNIT_ASSERT(orig.mValue == comp);
   
   // check output arguments
   CPPUNIT_ASSERT(pos == 3);
   CPPUNIT_ASSERT(count = comp.size());   
}


void CRangeUpdateTest::testDeleteSetter()
{   
   DSI::Private::ServerAttribute<std::vector<int> > orig;
   orig.mValue.push_back(1);
   orig.mValue.push_back(2);
   orig.mValue.push_back(3);
   orig.mValue.push_back(4);
   orig.mValue.push_back(5);
   orig.mValue.push_back(6);
   
   std::vector<int> updt;
   
   DSI::UpdateType up = DSI::UPDATE_DELETE;   
   
   int16_t pos = 2;
   int16_t count = 3;
   orig.set(updt, up, &pos, &count);
   
   std::vector<int> comp;
   comp.push_back(1);
   comp.push_back(2);
   comp.push_back(6);
      
   CPPUNIT_ASSERT(orig.mValue == comp);
   
   // check output arguments
   CPPUNIT_ASSERT(pos == 2);
   CPPUNIT_ASSERT(count = 3);
}


void CRangeUpdateTest::testDeleteSetterRest()
{   
   DSI::Private::ServerAttribute<std::vector<int> > orig;
   orig.mValue.push_back(1);
   orig.mValue.push_back(2);
   orig.mValue.push_back(3);
   orig.mValue.push_back(4);
   orig.mValue.push_back(5);
   orig.mValue.push_back(6);
   
   std::vector<int> updt;
   
   DSI::UpdateType up = DSI::UPDATE_DELETE;   
   
   int16_t pos = 2;
   int16_t count = -1;
   orig.set(updt, up, &pos, &count);
   
   std::vector<int> comp;
   comp.push_back(1);
   comp.push_back(2);   
      
   CPPUNIT_ASSERT(orig.mValue == comp);
   
   // check output arguments
   CPPUNIT_ASSERT(pos == 2);
   CPPUNIT_ASSERT(count = 4);
}


void CRangeUpdateTest::testReplaceSetter()
{   
   DSI::Private::ServerAttribute<std::vector<int> > orig;
   orig.mValue.push_back(1);
   orig.mValue.push_back(2);
   orig.mValue.push_back(3);
   orig.mValue.push_back(4);
   orig.mValue.push_back(5);
   orig.mValue.push_back(6);
   
   std::vector<int> updt;
   updt.push_back(33);
   updt.push_back(34);   
   
   DSI::UpdateType up = DSI::UPDATE_REPLACE;
   int16_t pos = 3;
   int16_t count = -1;
   orig.set(updt, up, &pos, &count);
   
   std::vector<int> comp;
   comp.push_back(1);
   comp.push_back(2);
   comp.push_back(3);
   comp.push_back(33);
   comp.push_back(34);
   comp.push_back(6);
   
   CPPUNIT_ASSERT(orig.mValue == comp);
   
   // check output arguments
   CPPUNIT_ASSERT(pos == 3);
   CPPUNIT_ASSERT(count = updt.size());
}
