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

#include <sys/timerfd.h>
#include <unistd.h>

#include "dsi/CCommEngine.hpp"

#include "CPingPongTestDSIProxy.hpp"
#include "CPingPongTestDSIStub.hpp"


class CUnblockTest : public CppUnit::TestFixture
{   
public:      
   
   enum Mode
   {
      MaskNoUnblock = (1<<4),
      
      Unblock = (1<<0),
      UnblockErrorResponse = (1<<1),
      UnblockErrorRequest = (1<<2),
      
      NoUnblock = Unblock | MaskNoUnblock,
      NoUnblockErrorResponse = UnblockErrorResponse | MaskNoUnblock,
      NoUnblockErrorRequest = UnblockErrorRequest | MaskNoUnblock,
      
      MaskDoubleRequest = (1<<5),
      
      DoubleRequestWithoutUnblock = MaskDoubleRequest | NoUnblock | MaskNoUnblock
   };
   
   CPPUNIT_TEST_SUITE(CUnblockTest);
      CPPUNIT_TEST(testUnblock);
      CPPUNIT_TEST(testUnblockErrorResponse);
      CPPUNIT_TEST(testUnblockErrorRequest);      
      
      CPPUNIT_TEST(testNoUnblock);            
      CPPUNIT_TEST(testNoUnblockErrorResponse);
      CPPUNIT_TEST(testNoUnblockErrorRequest);
      
      CPPUNIT_TEST(testDoubleRequestWithoutUnblock);
   CPPUNIT_TEST_SUITE_END();

public:
      
   void testUnblock();   
   void testUnblockErrorResponse();
   void testUnblockErrorRequest();      
   
   void testNoUnblock();   
   void testNoUnblockErrorResponse();
   void testNoUnblockErrorRequest();      
   
   void testDoubleRequestWithoutUnblock();
      
private:   

   void runTest(Mode mode);
};

CPPUNIT_TEST_SUITE_REGISTRATION(CUnblockTest);


// --------------------------------------------------------------------------------


class CPingPongTestClient : public CPingPongTestDSIProxy
{   
public:

   CPingPongTestClient(CUnblockTest::Mode mode)
    : CPingPongTestDSIProxy("testping")        
    , mMode(mode)
   {
      // NOOP
   }


   void componentConnected()
   {      
      requestPing(L"Hello");      
      mExpected = mMode;
      
      // send second request which will yield in a RESULT_REQUEST_BUSY, the first request will be 
      // answered after the RESULT_REQUEST_BUSY.
      if (mMode & CUnblockTest::MaskDoubleRequest)
      {
         requestPing(L"Hello again");         
      }
   }

   
   void componentDisconnected()
   {      
      CPPUNIT_ASSERT(engine());
      engine()->stop(0);
   }

   
   void responsePong(const std::wstring& /*message*/)
   {  
      CPPUNIT_ASSERT(mMode & CUnblockTest::Unblock);      
   }

   
   void responseInvalid(PingPongTest::UpdateIdEnum /*id*/)
   {      
      CPPUNIT_ASSERT(mMode & CUnblockTest::UnblockErrorResponse);      
   }


   void requestPingFailed(DSI::ResultType errType)
   {        
      if (mExpected & CUnblockTest::MaskDoubleRequest)
      {         
         CPPUNIT_ASSERT(errType == DSI::RESULT_REQUEST_BUSY);
         mExpected = mMode & ~CUnblockTest::MaskDoubleRequest;
      }
      else            
         CPPUNIT_ASSERT(mMode & CUnblockTest::UnblockErrorRequest);         
   }
      
   
private:     
   
   CUnblockTest::Mode mMode;
   int mExpected;
};


// -------------------------------------------------------------------------------------


class CPingPongTestServer : public CPingPongTestDSIStub
{
public:
   CPingPongTestServer(CUnblockTest::Mode mode)
    : CPingPongTestDSIStub("testping", false)    
    , mMode(mode)
    , mHandle(-1)    
   {
      mFd[0] = mFd[1] = -1;
      
      if (mMode & CUnblockTest::MaskDoubleRequest)
      {
         mFd[0] = ::timerfd_create(CLOCK_MONOTONIC, 0);
         CPPUNIT_ASSERT(mFd[0] > 0);         
      }
      else
      {
         CPPUNIT_ASSERT(::pipe(mFd) == 0);      
      }
   }   
   
   
   ~CPingPongTestServer()
   {     
      if (mFd[0] > 0)
         (void)::close(mFd[0]);
         
      if (mFd[1] > 0)
         (void)::close(mFd[1]);
   }

   
   void requestPing(const std::wstring& /*message*/)
   {      
      if (!(mMode & CUnblockTest::MaskNoUnblock))
      {      
         mHandle = unblockRequest();         
      }
            
      notify();      
   }
      
   
private:

   bool onNotify(DSI::CCommEngine::IOResult /*unused here*/)
   {    
      uint64_t expirations;
      (void)::read(mFd[0], &expirations, sizeof(expirations));      
      
      if (!(mMode & CUnblockTest::MaskNoUnblock))
      {   
         prepareResponse(mHandle);
         mHandle = -1;
      }
      
      if (mMode & CUnblockTest::Unblock)
      {         
         responsePong(L"Hi");
      }
      else if (mMode & CUnblockTest::UnblockErrorResponse)
      {
         sendError(PingPongTest::UPD_ID_responsePong);
      }
      else if (mMode & CUnblockTest::UnblockErrorRequest)
      {
         sendError(PingPongTest::UPD_ID_requestPing);
      }      
      
      CPPUNIT_ASSERT(engine());
      engine()->remove(*this);
      
      return false;
   }   

      
   void notify()
   {
      if (mFd[1] > 0)
      {
         char buf[1];
         (void)::write(mFd[1], buf, sizeof(buf));         
      }
      else
      {         
         // create interval timer for one-seconds intervals
         struct itimerspec spec = {
            { 0, 0 },
            { 0, 300000000 }
         };

         CPPUNIT_ASSERT(::timerfd_settime(mFd[0], 0, &spec, 0) == 0);
      }
      
      CPPUNIT_ASSERT(engine());
      engine()->addGenericDevice(mFd[0], DSI::CCommEngine::In, std::tr1::bind(&CPingPongTestServer::onNotify, this, std::tr1::placeholders::_1));
   }
   
   CUnblockTest::Mode mMode;   
   int mHandle;   
   int mFd[2];   
};


// -------------------------------------------------------------------------------------


void CUnblockTest::runTest(Mode mode)
{
   DSI::CCommEngine engine;
   
   CPingPongTestServer serv(mode);
   engine.add(serv);
   
   CPingPongTestClient clnt(mode);
   engine.add(clnt);
   
   CPPUNIT_ASSERT(engine.run() == 0);   
}


void CUnblockTest::testUnblock()
{
   runTest(Unblock);   
}


void CUnblockTest::testUnblockErrorResponse()
{
   runTest(UnblockErrorResponse);   
}


void CUnblockTest::testUnblockErrorRequest()
{
   runTest(UnblockErrorRequest);   
}


void CUnblockTest::testNoUnblock()
{
   runTest(NoUnblock);
}


void CUnblockTest::testNoUnblockErrorResponse()
{
   runTest(NoUnblockErrorResponse);
}


void CUnblockTest::testNoUnblockErrorRequest()
{
   runTest(NoUnblockErrorRequest);
}


void CUnblockTest::testDoubleRequestWithoutUnblock()
{
   runTest(DoubleRequestWithoutUnblock);
}
