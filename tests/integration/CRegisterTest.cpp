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

#include "CRegisterTestDSIProxy.hpp"
#include "CRegisterTestDSIStub.hpp"
#include "CServicebroker.hpp"


/**
 * This test verifies 
 *  - if the register pattern works when register on an information
 *  - if the information is also sent to clients which set ordinary notifications
 */
class CRegisterTest : public CppUnit::TestFixture
{   
public:      
   
   CPPUNIT_TEST_SUITE(CRegisterTest);
      CPPUNIT_TEST(testRegister);            
   CPPUNIT_TEST_SUITE_END();

public:
      
   void testRegister();            
};

CPPUNIT_TEST_SUITE_REGISTRATION(CRegisterTest);


// --------------------------------------------------------------------------------


class CRegisterTestClient : public CRegisterTestDSIProxy
{   
public:

   CRegisterTestClient()
    : CRegisterTestDSIProxy("register")      
    , mCalls(0)
    , mCount(5)
   {
      // NOOP
   }

   
   void componentConnected()
   {
      RegisterTest::CUpdateIdVector updates;
      updates.push_back(RegisterTest::UPD_ID_informationValueChanged);
      
      registerRegisterValue(updates, true);
      
      // flip the registered value some times
      requestEval();      
   }
   
   
   void responseEvaled()
   {
      if (--mCount > 0)
      {         
         // flip the registered value some times
         requestEval();
      }
   }
      
   
   void componentDisconnected()
   {          
      CPPUNIT_ASSERT(mCalls == 3);   
      
      CPPUNIT_ASSERT(engine());
      engine()->stop(0);           
   }
   
   
   void registerRegisterValueFailed( DSI::ResultType /*errType*/ )
   {
      CPPUNIT_FAIL("should never be called");
   }
   
   
   void informationValueChanged(bool value) 
   {               
      ++mCalls;
      assert(value == true);
   }
   
   
   void onValueUpdate(bool /*Value*/, DSI::DataStateType /*state*/)
   {
      CPPUNIT_FAIL("should never be called since we did not set a notifion on it");
   }
   
private:
   int mCalls;
   int mCount;
};


// -------------------------------------------------------------------------------------


class CRegisterTestClientA : public CRegisterTestDSIProxy
{   
public:

   CRegisterTestClientA()
    : CRegisterTestDSIProxy("register")      
    , mCalls(0)    
   {
      // NOOP
   }

   
   void componentConnected()
   {
      notifyOnInformationValueChanged();      
   }
   
   
   void responseEvaled()
   {
      CPPUNIT_FAIL("never called");
   }
      
   
   void componentDisconnected()
   {                
      CPPUNIT_ASSERT(mCalls > 3);         
   }
   
   
   void registerRegisterValueFailed( DSI::ResultType /*errType*/ )
   {
      CPPUNIT_FAIL("should never be called");
   }
   
   
   void informationValueChanged(bool /*value*/) 
   {                    
      ++mCalls;      
   }
   
   
   void onValueUpdate(bool /*Value*/, DSI::DataStateType /*state*/)
   {
      CPPUNIT_FAIL("should never be called since we did not set a notifion on it");
   }
   
private:
   int mCalls;   
};


// -------------------------------------------------------------------------------------


class CRegisterTestServer : public CRegisterTestDSIStub
{
public:

   CRegisterTestServer()
    : CRegisterTestDSIStub("register", false)          
    , mCount(5)
   {
      setValue(true);
   }   
   
   
   ~CRegisterTestServer()
   { 
      // NOOP
   }
   
   
   void requestEval()
   {            
      setValue(!getValue());
      
      trySendInformation();
      
      if (--mCount > 0)      
      {
         responseEvaled();
      }
      else      
      {
         engine()->remove(*this);
      }
   }
   
   
   void registerRegisterValue(bool value)
   {      
      mRegisteredClients[registerCurrentSession()] = value;
      trySendInformation();
   }
  
  
   void unregisterRegisterValue(int32_t sessionID)
   {
      mRegisteredClients.erase(mRegisteredClients.find(sessionID));
   }
   
private:

   void trySendInformation()
   {      
      for(std::map<int32_t, bool>::iterator iter = mRegisteredClients.begin(); iter != mRegisteredClients.end(); ++iter)
      {         
         if (iter->second == getValue())
         {       
            addActiveSession(iter->first);            
         }
      }
            
      informationValueChanged(getValue());
      clearActiveSessions();
   }

   std::map<int32_t, bool> mRegisteredClients; 
   int mCount;
};


// -------------------------------------------------------------------------------------


void CRegisterTest::testRegister()
{
   DSI::CCommEngine engine;
   
   CRegisterTestServer serv;
   engine.add(serv);
   
   CRegisterTestClientA clnt_a;
   engine.add(clnt_a);
      
   CRegisterTestClient clnt;
   engine.add(clnt);
      
   CPPUNIT_ASSERT(engine.run() == 0);   
}


