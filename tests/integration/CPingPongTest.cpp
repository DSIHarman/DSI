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
#include "CServicebroker.hpp"


class CPingPongTestBase
{
public:

   enum Mode
   {
      NormalResponse,
      ErrorResponse,
      ErrorRequest      
   };
};


template<bool ServerInSeparateThread>
class CPingPongTest : public CppUnit::TestFixture, protected CPingPongTestBase
{   
public:

   CPPUNIT_TEST_SUITE(CPingPongTest);
      CPPUNIT_TEST(testPingPong);
      CPPUNIT_TEST(testPingPongErrorResponse);
      CPPUNIT_TEST(testPingPongErrorRequest);
      CPPUNIT_TEST(testPingPongTCP);         
   CPPUNIT_TEST_SUITE_END();

public:
   
   void testPingPong();      
   void testPingPongErrorResponse();      
   void testPingPongErrorRequest();      
   void testPingPongTCP();       
   
private:   
   void testPingPongImpl(Mode m, bool tcp = false);      
};

CPPUNIT_TEST_SUITE_REGISTRATION(CPingPongTest<true>);   // server in separate thread
CPPUNIT_TEST_SUITE_REGISTRATION(CPingPongTest<false>);   // server within same thread as client


// --------------------------------------------------------------------------------

const std::wstring CLIENT_MESSAGE(L"Message from proxy");
const std::wstring SERVER_MESSAGE(L"A message from stub");


// FIXME add a servicebroker check function somewhere and start automatically if necessary
class CPingPongTestClient : public CPingPongTestDSIProxy
{
public:

   CPingPongTestClient(CPingPongTestBase::Mode mode)
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
   
   void responsePong(const std::wstring& message)
   {      
      if (mMode == CPingPongTestBase::NormalResponse)
      {
         CPPUNIT_ASSERT(SERVER_MESSAGE == message);
      }
      else
      {
         CPPUNIT_FAIL("unexpected response from server");
      }
            
      CPPUNIT_ASSERT(engine());
      engine()->stop(0);
   }

   
   void responseInvalid(PingPongTest::UpdateIdEnum id)
   {      
      if (mMode == CPingPongTestBase::ErrorResponse)
      {
         CPPUNIT_ASSERT(id == PingPongTest::UPD_ID_responsePong);         
      }
      else
      {
         CPPUNIT_FAIL("unexpected response from server");
      }
            
      CPPUNIT_ASSERT(engine());
      engine()->stop(0);      
   }


   void requestPingFailed(DSI::ResultType errType)
   {      
      if (mMode == CPingPongTestBase::ErrorRequest)
      {
         CPPUNIT_ASSERT(errType == DSI::RESULT_REQUEST_ERROR);
      }
      else
      {
         CPPUNIT_FAIL("unexpected response from server");
      }
            
      CPPUNIT_ASSERT(engine());
      engine()->stop(0);      
   }
   
private:
   CPingPongTestBase::Mode mMode;
};


// -------------------------------------------------------------------------------------


class CPingPongTestServer : public CPingPongTestDSIStub
{
public:
   CPingPongTestServer(CPingPongTestBase::Mode mode)
    : CPingPongTestDSIStub("testping", ::getenv("DSI_FORCE_TCP") ? true : false/*enable tcpip*/)    
    , mMode(mode)
   {
      // NOOP
   }

   
   void requestPing(const std::wstring& message)
   {
      CPPUNIT_ASSERT(CLIENT_MESSAGE == message);      
      
      if (mMode == CPingPongTestBase::NormalResponse)
      {
         responsePong(SERVER_MESSAGE);
      }
      else if (mMode == CPingPongTestBase::ErrorRequest)
      {               
         sendError(PingPongTest::UPD_ID_requestPing);
      }
      else
      {         
         sendError(PingPongTest::UPD_ID_responsePong);
      }      
   }   
   
private:
   CPingPongTestBase::Mode mMode;
};


// -------------------------------------------------------------------------------------

typedef std::tr1::tuple<DSI::CCommEngine*, CPingPongTestBase::Mode> tThreadArgsType;

void* serverRunner(void* arg)
{      
   DSI::CCommEngine* engine;
   CPingPongTestBase::Mode mode;
   std::tr1::tie(engine, mode) = *((tThreadArgsType*)arg);   
   
   CPingPongTestServer server(mode);
   engine->add(server);
         
   CPPUNIT_ASSERT_EQUAL(0, engine->run());
   return 0;
}


// -------------------------------------------------------------------------------------


template<bool ServerInSeparateThread>
void CPingPongTest<ServerInSeparateThread>::testPingPongImpl(Mode mode, bool /*tcp*/)
{   
   DSI::CCommEngine engine;
   DSI::CCommEngine serverEngine;
   
   std::unique_ptr<CPingPongTestServer> server(nullptr);
   pthread_t tid;   
   
   if (ServerInSeparateThread)
   {      
      tThreadArgsType args(&serverEngine, mode);      
      CPPUNIT_ASSERT(::pthread_create(&tid, nullptr, &serverRunner, &args) == 0);
   }
   else
   {
      server.reset(new CPingPongTestServer(mode));
      engine.add(*server);
   }
   
   CPingPongTestClient client(mode);         
   engine.add(client);         
   
   CPPUNIT_ASSERT_EQUAL(0, engine.run());
   
   if (ServerInSeparateThread)
   {
      serverEngine.stop(0);
      CPPUNIT_ASSERT(::pthread_join(tid, 0) == 0);
   }
}


template<bool ServerInSeparateThread>
void CPingPongTest<ServerInSeparateThread>::testPingPong()
{
   CPPUNIT_ASSERT(!::unsetenv("DSI_FORCE_TCP"));   
   testPingPongImpl(NormalResponse);   
}


template<bool ServerInSeparateThread>
void CPingPongTest<ServerInSeparateThread>::testPingPongErrorResponse()
{
   CPPUNIT_ASSERT(!::unsetenv("DSI_FORCE_TCP"));      
   testPingPongImpl(ErrorResponse);   
}


template<bool ServerInSeparateThread>
void CPingPongTest<ServerInSeparateThread>::testPingPongErrorRequest()
{
   CPPUNIT_ASSERT(!::unsetenv("DSI_FORCE_TCP"));      
   testPingPongImpl(ErrorRequest);
}


template<bool ServerInSeparateThread>
void CPingPongTest<ServerInSeparateThread>::testPingPongTCP()
{   
   CPPUNIT_ASSERT(!::setenv("DSI_FORCE_TCP", "1", 1));   
   testPingPongImpl(NormalResponse, true);   
   CPPUNIT_ASSERT(!::unsetenv("DSI_FORCE_TCP")); 
}
