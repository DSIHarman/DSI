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
#include <unistd.h>

#include "dsi/CCommEngine.hpp"

#include "CPingPongTestDSIStub.hpp"
#include "CServicebroker.hpp"


class CTCPRegistrationTest : public CppUnit::TestFixture
{   
public:

   CPPUNIT_TEST_SUITE(CTCPRegistrationTest);      
      CPPUNIT_TEST(testTCPBinding);            
   CPPUNIT_TEST_SUITE_END();

public:
      
   void testTCPBinding();   
};

CPPUNIT_TEST_SUITE_REGISTRATION(CTCPRegistrationTest);   


// -------------------------------------------------------------------------------------


class CPingPongTestServer : public CPingPongTestDSIStub
{
public:
   CPingPongTestServer()
    : CPingPongTestDSIStub("testping1", true)        
   {
      // NOOP
   }

   
   void requestPing(const std::wstring& /*message*/)
   {
      // NOOP
   }      
};


// -------------------------------------------------------------------------------------


void* serverRunner(void* arg)
{      
   DSI::CCommEngine* engine = (DSI::CCommEngine*)arg;      
   
   CPingPongTestServer server;
   engine->add(server);
         
   CPPUNIT_ASSERT_EQUAL(0, engine->run());
   return nullptr;
}


// -------------------------------------------------------------------------------------


void CTCPRegistrationTest::testTCPBinding()
{   
   CPPUNIT_ASSERT(!::setenv("DSI_COMMENGINE_PORT", "7766", 0));         
   
   DSI::CCommEngine serverEngine;   
   pthread_t tid;      
   
   CPPUNIT_ASSERT(::pthread_create(&tid, nullptr, &serverRunner, &serverEngine) == 0);
   
   // let the server completely enter it's eventloop, otherwise we could call 
   // stop (below) before the server is ready to handle it and therefore block 
   // within the join call.   
   ::sleep(1);        
   
   STCPConnectionInfo info;
   
   // wait for server thread to get ready          
   int rc;
   while((rc = SBAttachInterfaceTCP(DSI::CServicebroker::GetSBHandle(), "testping1.PingPongTest", 
                                    PingPongTest::MAJOR_VERSION, PingPongTest::MINOR_VERSION, &info)) == FNDUnknownInterface)
   {
      ::usleep(100);
   }                                    
   
   CPPUNIT_ASSERT(rc == 0);
   
   SBDetachInterface(DSI::CServicebroker::GetSBHandle(), info.clientID);
   CPPUNIT_ASSERT(info.socket.port == ::atoi(::getenv("DSI_COMMENGINE_PORT")));
   
   CPPUNIT_ASSERT(!::unsetenv("DSI_COMMENGINE_PORT"));   
            
   serverEngine.stop(0);   
   CPPUNIT_ASSERT(::pthread_join(tid, 0) == 0);         
}
