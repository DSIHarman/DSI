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


/**
 * Tests the following stuff for 'onChange' and 'always' attributes: 
 *
 * 1) Attribute is already initialized when notification is set by the client
 *    -> client will get an update event and the clients value will be set.
 *
 * 2) Attribute is not initialized when notification is set by the client
 *    -> client will not get any update event.
 *
 * 3) Like 2), but the value is initialized after the notification is set by the client 
 *    -> client should get an update event now. 
 * 
 * 4) Count update events for both attributes notification types.
 *
 * 5) Invalidate the attribute on server side again.
 *
 * 6) server must send invalid attribute state to client if client attaches to
 *    an invalid attribute (in contrast to an unset one)
 */
class CAttributesTest : public CppUnit::TestFixture
{   
public:      
    
   enum Mode
   {
      UnsetAttributeOnStartup         = (1<<0),
      ValidAttributeOnStartup         = (1<<1),
      InitializeAttributeAfterStartup = (1<<2),
      UpdateEventsCount               = (1<<3),
      Invalidate                      = (1<<4),
      InvalidAttributeOnStartup       = (1<<5),
            
      MaskAnotherIntAttribute         = (1<<7)
   };
      
   CPPUNIT_TEST_SUITE(CAttributesTest);
      CPPUNIT_TEST(testInvalidAttributeOnStartup);
      CPPUNIT_TEST(testUnsetAttributeOnStartup);
      CPPUNIT_TEST(testValidAttributeOnStartup);  
      CPPUNIT_TEST(testInitializeAttributeAfterStartup);  
      
      CPPUNIT_TEST(testAnotherUnsetAttributeOnStartup);
      CPPUNIT_TEST(testAnotherValidAttributeOnStartup);  
      CPPUNIT_TEST(testAnotherInitializeAttributeAfterStartup);  
      
      CPPUNIT_TEST(testUpdateEventsCount);      
      CPPUNIT_TEST(testAnotherUpdateEventsCount);      
      
      CPPUNIT_TEST(testInvalidate);
   CPPUNIT_TEST_SUITE_END();

public:
      
   void testInvalidAttributeOnStartup();
   void testUnsetAttributeOnStartup();      
   void testValidAttributeOnStartup();      
   void testInitializeAttributeAfterStartup();
   
   void testAnotherUnsetAttributeOnStartup();
   void testAnotherValidAttributeOnStartup();  
   void testAnotherInitializeAttributeAfterStartup();  
      
   void testUpdateEventsCount();
   void testAnotherUpdateEventsCount();
   
   void testInvalidate();
   
private:
   void runTest(int mode);
};

CPPUNIT_TEST_SUITE_REGISTRATION(CAttributesTest);


// --------------------------------------------------------------------------------


class CAttributesTestClient : public CAttributesTestDSIProxy
{   
public:

   CAttributesTestClient(int mode)
    : CAttributesTestDSIProxy("attributes")        
    , mMode(mode)
    , mUpdateCount(0) 
    , mInvalidated(false)    
   {
      // NOOP
   }

   
   ~CAttributesTestClient()
   {
      CPPUNIT_ASSERT(!isMyIntAttrValid());
      CPPUNIT_ASSERT(!isAnotherIntAttrValid());
   }
   
   
   void componentConnected()
   {     
      CPPUNIT_ASSERT(!isAnotherIntAttrValid());   
      CPPUNIT_ASSERT(!isMyIntAttrValid());
      
      if (mMode & CAttributesTest::MaskAnotherIntAttribute)
      {        
         notifyOnAnotherIntAttr();      
      }
      else
      {        
         notifyOnMyIntAttr();      
      }
      
      if (mMode == CAttributesTest::InitializeAttributeAfterStartup)          
      {
         requestInit();
      }
      
      if (mMode & CAttributesTest::UpdateEventsCount)
      {
         // five times so the event may be sent 5 times back (with 'always' notification type)
         requestInit();
         requestInit();
         requestInit();
         requestInit();
         requestInit();
      }
      
      if (mMode & CAttributesTest::Invalidate)
      {
         requestInit();
         requestInvalidate();
      }
            
      requestStop();            
   }
   
   
   void componentDisconnected()
   {
      // NOOP
   }
   
   
   void onMyIntAttrUpdate( int64_t value, DSI::DataStateType state)
   {      
      if (state == DSI::DATA_INVALID)
      {               
         mInvalidated = true;         
      }
      else
      {           
         ++mUpdateCount;
         
         CPPUNIT_ASSERT(value == 42ll);
      }
   }
   
   
   void onAnotherIntAttrUpdate( int32_t value, DSI::DataStateType /*state*/)
   {
      ++mUpdateCount;      
      
      // check order of function calls
      CPPUNIT_ASSERT(getAnotherIntAttr() == 42);
      CPPUNIT_ASSERT(value == 42);
   }   
   
   
   void responseStopped()
   {
      if (mMode & CAttributesTest::MaskAnotherIntAttribute)
      {      
         // check the other one never changed state
         CPPUNIT_ASSERT(!isMyIntAttrValid());
         
         if (mMode & CAttributesTest::UnsetAttributeOnStartup)
         {
            CPPUNIT_ASSERT(!isAnotherIntAttrValid());
            CPPUNIT_ASSERT(mUpdateCount == 0);
         }

         if (mMode & CAttributesTest::ValidAttributeOnStartup)
         {
            CPPUNIT_ASSERT(isAnotherIntAttrValid());
            CPPUNIT_ASSERT(getAnotherIntAttr() == 42);
            CPPUNIT_ASSERT(mUpdateCount == 1);
         }
         
         if (mMode & CAttributesTest::UpdateEventsCount)
         {
            CPPUNIT_ASSERT(isAnotherIntAttrValid());
            CPPUNIT_ASSERT(getAnotherIntAttr() == 42);            
            //CPPUNIT_ASSERT(mUpdateCount == 5);
         }
                  
         // test not implemented...
         CPPUNIT_ASSERT(!(mMode & CAttributesTest::Invalidate));
      }
      else
      {
         // check the other one never changed state
         CPPUNIT_ASSERT(!isAnotherIntAttrValid());   
       
         if (mMode & CAttributesTest::InvalidAttributeOnStartup)
         {
            CPPUNIT_ASSERT(mInvalidated);
         }
                
         if (mMode & CAttributesTest::UnsetAttributeOnStartup)
         {
            CPPUNIT_ASSERT(!isMyIntAttrValid());
            CPPUNIT_ASSERT(mUpdateCount == 0);
         }

         if (mMode & CAttributesTest::ValidAttributeOnStartup)
         {
            CPPUNIT_ASSERT(isMyIntAttrValid());
            CPPUNIT_ASSERT(getMyIntAttr() == 42);
            CPPUNIT_ASSERT(mUpdateCount == 1);
         }
         
         if (mMode & CAttributesTest::UpdateEventsCount)
         {
            CPPUNIT_ASSERT(isMyIntAttrValid());
            CPPUNIT_ASSERT(getMyIntAttr() == 42);
            CPPUNIT_ASSERT(mUpdateCount == 1);
         }
         
         if (mMode & CAttributesTest::Invalidate)
         {
            CPPUNIT_ASSERT(mInvalidated == true);
            CPPUNIT_ASSERT(!isMyIntAttrValid());
         }
      }      
      
      CPPUNIT_ASSERT(engine());
      engine()->stop();
   }
   
private:

   int mMode;   
   int mUpdateCount;
   bool mInvalidated;
};


// -------------------------------------------------------------------------------------


class CAttributesTestServer : public CAttributesTestDSIStub
{
public:
   CAttributesTestServer(int mode)
    : CAttributesTestDSIStub("attributes", false)  
    , mMode(mode)    
   {
      CPPUNIT_ASSERT(getAnotherIntAttrState() == DSI::DATA_NOT_AVAILABLE);         
      CPPUNIT_ASSERT(getMyIntAttrState() == DSI::DATA_NOT_AVAILABLE);               
      
      if (mMode & CAttributesTest::ValidAttributeOnStartup)
      {       
         setAnotherIntAttr(42);         
         setMyIntAttr(42);         
      }
      
      if (mMode & CAttributesTest::InvalidAttributeOnStartup)
      {
         invalidateMyIntAttr();
         invalidateAnotherIntAttr();
      }
   }   
   
   
   ~CAttributesTestServer()
   {     
      // NOOP
   }

   
   void requestInit()
   {                 
      setAnotherIntAttr(42);      
      setMyIntAttr(42);      
   }
   
   
   void requestInvalidate()
   {        
      invalidateMyIntAttr();
      invalidateAnotherIntAttr();      
   }
   
   
   void requestStop()
   {      
      responseStopped();      
   }
   
private:
   int mMode;      
};


// -------------------------------------------------------------------------------------


void CAttributesTest::runTest(int mode)
{
   DSI::CCommEngine engine;
   
   CAttributesTestServer serv(mode);
   engine.add(serv);
   
   CAttributesTestClient clnt(mode);
   engine.add(clnt);
   
   CPPUNIT_ASSERT(engine.run() == 0);      
}


void CAttributesTest::testInvalidAttributeOnStartup()
{
   runTest(InvalidAttributeOnStartup);
}


void CAttributesTest::testUnsetAttributeOnStartup()
{
   runTest(UnsetAttributeOnStartup);
}


void CAttributesTest::testValidAttributeOnStartup()
{
   runTest(ValidAttributeOnStartup);
}


void CAttributesTest::testInitializeAttributeAfterStartup()
{
   runTest(InitializeAttributeAfterStartup);
}


void CAttributesTest::testAnotherUnsetAttributeOnStartup()
{
   runTest(UnsetAttributeOnStartup | MaskAnotherIntAttribute);
}


void CAttributesTest::testAnotherValidAttributeOnStartup()
{
   runTest(ValidAttributeOnStartup | MaskAnotherIntAttribute);
}


void CAttributesTest::testAnotherInitializeAttributeAfterStartup()
{
   runTest(InitializeAttributeAfterStartup | MaskAnotherIntAttribute);
}
 
 
void CAttributesTest::testUpdateEventsCount()
{
   runTest(UpdateEventsCount);
}


void CAttributesTest::testAnotherUpdateEventsCount()
{
   runTest(UpdateEventsCount | MaskAnotherIntAttribute);
}


void CAttributesTest::testInvalidate()
{
   runTest(Invalidate);
}
