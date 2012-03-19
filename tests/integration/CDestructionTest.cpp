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
#include <pthread.h>
#include <arpa/inet.h>
#include <tr1/tuple>

#include "dsi/CCommEngine.hpp"

#include "CPingPongTestDSIProxy.hpp"
#include "CPingPongTestDSIStub.hpp"


class CDestructionTest : public CppUnit::TestFixture
{   
public:

   enum Mode
   {
      DestructClient,
      DestructServer,
      Noop      
   };

   CPPUNIT_TEST_SUITE(CDestructionTest);
      CPPUNIT_TEST(testClient);
      CPPUNIT_TEST(testServer);      
      CPPUNIT_TEST(testBeforeRun);
   CPPUNIT_TEST_SUITE_END();

public:
   
   void testClient();      
   void testServer();   
   
   void testBeforeRun();
     
};

CPPUNIT_TEST_SUITE_REGISTRATION(CDestructionTest);  


// --------------------------------------------------------------------------------

const std::wstring CLIENT_MESSAGE(L"Message from proxy");
const std::wstring SERVER_MESSAGE(L"A message from stub");


class CDestructionTestClient : public CPingPongTestDSIProxy
{
public:

   CDestructionTestClient(CDestructionTest::Mode mode)
    : CPingPongTestDSIProxy("testping")    
    , mMode(mode)
   {
      // NOOP
   }


   void componentConnected()
   {      
      requestPing(CLIENT_MESSAGE);
   }

   
   void componentDisconnected()
   {      
      // NOOP
   }
   
   
   void responsePong(const std::wstring& /*message*/)
   {      
      if (mMode == CDestructionTest::DestructClient)
      {
         DSI::CCommEngine* theEngine = engine();
         CPPUNIT_ASSERT(theEngine);
         
         delete this;
         
         theEngine->stop();
      }      
   }   
   
private:
   CDestructionTest::Mode mMode;
};


// -------------------------------------------------------------------------------------


class CDestructionTestServer : public CPingPongTestDSIStub
{
public:

   CDestructionTestServer(CDestructionTest::Mode mode)
    : CPingPongTestDSIStub("testping")    
    , mMode(mode)
   {
      // NOOP
   }
   
   
   void requestPing(const std::wstring& /*message*/)
   {      
      if (mMode == CDestructionTest::DestructServer)
      {
         DSI::CCommEngine* theEngine = engine();
         CPPUNIT_ASSERT(theEngine);
         
         responsePong(SERVER_MESSAGE);
         delete this;    

         theEngine->stop();
      }
      else
      {
         responsePong(SERVER_MESSAGE);      
      }
   }   
   
private:
   CDestructionTest::Mode mMode;
};


// -------------------------------------------------------------------------------------


void CDestructionTest::testBeforeRun()
{   
   DSI::CCommEngine engine;
            
   {
      CDestructionTestClient client(Noop);
      engine.add(client);         
   }
   
   {
      CDestructionTestServer server(Noop);         
      engine.add(server);               
   }
}


void CDestructionTest::testServer()
{   
   DSI::CCommEngine engine;   
   
   CDestructionTestClient client(DestructServer);
   engine.add(client);            
      
   CDestructionTestServer* server = new CDestructionTestServer(DestructServer);         
   engine.add(*server);               

   CPPUNIT_ASSERT(engine.run() == 0);
}


void CDestructionTest::testClient()
{   
   DSI::CCommEngine engine;   
   
   CDestructionTestClient* client = new CDestructionTestClient(DestructClient);
   engine.add(*client);            
      
   CDestructionTestServer server(DestructClient);         
   engine.add(server);               

   CPPUNIT_ASSERT(engine.run() == 0);
}

