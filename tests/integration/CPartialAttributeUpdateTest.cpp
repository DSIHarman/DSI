/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <cstdlib>

#include "dsi/CCommEngine.hpp"

#include "CAttributesTestDSIProxy.hpp"
#include "CAttributesTestDSIStub.hpp"
#include "../../src/base/utf8.hpp"


/**
 * Tests the following use-cases:
 *
 * 1) partial delete
 * 2) partial replace
 * 3) partial insert
 * 
 */
class CPartialAttributeUpdateTest : public CppUnit::TestFixture
{   
public:      
    
   enum Mode
   {
      Complete = 0,
      Delete = 1,
      Replace,
      Insert
   };
      
   CPPUNIT_TEST_SUITE(CPartialAttributeUpdateTest);
      CPPUNIT_TEST(testComplete);
      CPPUNIT_TEST(testDelete);
      CPPUNIT_TEST(testReplace);  
      CPPUNIT_TEST(testInsert);        
   CPPUNIT_TEST_SUITE_END();

public:
      
   void testComplete();      
   void testDelete();      
   void testReplace();      
   void testInsert();
      
private:
   void runTest(int mode);
};

CPPUNIT_TEST_SUITE_REGISTRATION(CPartialAttributeUpdateTest);


// --------------------------------------------------------------------------------


class CPartialAttributeUpdateTestClient : public CAttributesTestDSIProxy
{   
public:

   CPartialAttributeUpdateTestClient(int mode)
    : CAttributesTestDSIProxy("attributes")        
    , mMode(mode)
    , mCount(0)    
   {
      // NOOP
   }

   
   void componentConnected()
   {     
      CPPUNIT_ASSERT(!isMyVectorAttributeValid());   
                  
      notifyOnMyVectorAttribute();                        
      requestInit();                
      requestStop();            
   }
   
   
   void componentDisconnected()
   {
      // NOOP
   }
   
   
   void onMyVectorAttributeUpdate(const AttributesTest::MyVector& attribute, DSI::DataStateType state, DSI::UpdateType type, int16_t position, int16_t count)
   {            
      ++mCount;
           
      // drop first package on attribute update
      if (mMode != CPartialAttributeUpdateTest::Complete && mCount == 1)      
         return;                     
      
      if (mMode == CPartialAttributeUpdateTest::Complete)
      {           
         CPPUNIT_ASSERT(state == DSI::DATA_OK);         
         CPPUNIT_ASSERT(type == DSI::UPDATE_COMPLETE);         
         CPPUNIT_ASSERT(position == 0);         
         CPPUNIT_ASSERT(count == 3);         
         
         std::vector<std::wstring> comp;
         comp.push_back(L"einundzwanzig");
         comp.push_back(L"zweiundzwanzig");
         comp.push_back(L"dreiundzwanzig");
         
         CPPUNIT_ASSERT(comp == attribute);         
      }
      else if (mMode == CPartialAttributeUpdateTest::Delete)
      {                           
         CPPUNIT_ASSERT(state == DSI::DATA_OK);
         CPPUNIT_ASSERT(type == DSI::UPDATE_DELETE);
         CPPUNIT_ASSERT(position == 0);
         CPPUNIT_ASSERT(count == 6);         
         
         CPPUNIT_ASSERT(attribute.empty());         
      }
      else if (mMode == CPartialAttributeUpdateTest::Replace)
      {
         CPPUNIT_ASSERT(state == DSI::DATA_OK);
         CPPUNIT_ASSERT(type == DSI::UPDATE_REPLACE);
         CPPUNIT_ASSERT(position == 2);
         CPPUNIT_ASSERT(count == 3);         
         
         std::vector<std::wstring> comp;
         comp.push_back(L"Eins");
         comp.push_back(L"Zwei");
         comp.push_back(L"einundzwanzig");
         comp.push_back(L"zweiundzwanzig");
         comp.push_back(L"dreiundzwanzig");
         comp.push_back(L"Sechs");
                  
         CPPUNIT_ASSERT(attribute == comp);         
      }
      else if (mMode == CPartialAttributeUpdateTest::Insert)
      {
         CPPUNIT_ASSERT(state == DSI::DATA_OK);
         CPPUNIT_ASSERT(type == DSI::UPDATE_INSERT);
         CPPUNIT_ASSERT(position == 2);
         CPPUNIT_ASSERT(count == 3);    
         
         std::vector<std::wstring> comp;
         comp.push_back(L"Eins");
         comp.push_back(L"Zwei");
         comp.push_back(L"einundzwanzig");
         comp.push_back(L"zweiundzwanzig");
         comp.push_back(L"dreiundzwanzig");
         comp.push_back(L"Drei");
         comp.push_back(L"Vier");
         comp.push_back(L"Fuenf");
         comp.push_back(L"Sechs");
         
         CPPUNIT_ASSERT(attribute == comp);
      }      
   }   
   
   
   void responseStopped()
   {                  
      CPPUNIT_ASSERT(mCount == (mMode == CPartialAttributeUpdateTest::Complete ? 1 : 2));
      
      CPPUNIT_ASSERT(engine());
      engine()->stop();
   }
   
private:

   int mMode;   
   int mCount;
};


// -------------------------------------------------------------------------------------


class CPartialAttributeUpdateTestServer : public CAttributesTestDSIStub
{
public:
   CPartialAttributeUpdateTestServer(int mode)
    : CAttributesTestDSIStub("attributes", false)  
    , mMode(mode)    
   {           
      CPPUNIT_ASSERT(!isMyVectorAttributeValid());      
      CPPUNIT_ASSERT(getMyVectorAttributeState() == DSI::DATA_NOT_AVAILABLE);         
      
      
      if (mMode != CPartialAttributeUpdateTest::Complete)
      {
         std::vector<std::wstring> v;
         v.push_back(L"Eins");
         v.push_back(L"Zwei");
         v.push_back(L"Drei");
         v.push_back(L"Vier");
         v.push_back(L"Fuenf");
         v.push_back(L"Sechs");
         
         setMyVectorAttribute(v);        
         CPPUNIT_ASSERT(isMyVectorAttributeValid());    
      }
   }   
   
   
   ~CPartialAttributeUpdateTestServer()
   {     
      // NOOP
   }

   
   void requestInit()
   {                  
      if (mMode == CPartialAttributeUpdateTest::Complete)
      {      
         std::vector<std::wstring> v;
         v.push_back(L"einundzwanzig");
         v.push_back(L"zweiundzwanzig");
         v.push_back(L"dreiundzwanzig");
                  
         setMyVectorAttribute(v);         
      }
      else if (mMode == CPartialAttributeUpdateTest::Delete)
      {         
         setMyVectorAttribute(getMyVectorAttribute(), DSI::UPDATE_DELETE);
      }
      else if (mMode == CPartialAttributeUpdateTest::Replace)
      {
         std::vector<std::wstring> v;
         v.push_back(L"einundzwanzig");
         v.push_back(L"zweiundzwanzig");
         v.push_back(L"dreiundzwanzig");
         
         setMyVectorAttribute(v, DSI::UPDATE_REPLACE, 2, 3);
      }
      else if (mMode == CPartialAttributeUpdateTest::Insert)
      {
         std::vector<std::wstring> v;
         v.push_back(L"einundzwanzig");
         v.push_back(L"zweiundzwanzig");
         v.push_back(L"dreiundzwanzig");
         
         setMyVectorAttribute(v, DSI::UPDATE_INSERT, 2, 3);
      }
   }
   
   
   void requestInvalidate()
   {        
      // NOOP
   }
   
   
   void requestStop()
   {            
      responseStopped();      
   }
   
private:
   int mMode;      
};


// -------------------------------------------------------------------------------------


void CPartialAttributeUpdateTest::runTest(int mode)
{
   DSI::CCommEngine engine;
   
   CPartialAttributeUpdateTestServer serv(mode);
   engine.add(serv);
   
   CPartialAttributeUpdateTestClient clnt(mode);
   engine.add(clnt);
   
   CPPUNIT_ASSERT(engine.run() == 0);   
}


void CPartialAttributeUpdateTest::testComplete()
{
   runTest(Complete);
}


void CPartialAttributeUpdateTest::testDelete()
{
   runTest(Delete);
}


void CPartialAttributeUpdateTest::testReplace()
{
   runTest(Replace);
}


void CPartialAttributeUpdateTest::testInsert()
{
   runTest(Insert);
}
