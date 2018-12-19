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


class CMultiPingPongTest : public CppUnit::TestFixture
{   
public:   
   
   enum Mode
   {
      DoubleResponse = 0,
      DoubleResponseError = 1,
      DoubleRequestError = 2,

      ResponseAndResponseError = 3,
      ResponseErrorAndResponse = 4,
      
      ResponseAndRequestError = 5,
      RequestErrorAndResponse = 6,
      
      ResponseErrorAndRequestError = 7,
      RequestErrorAndResponseError = 8,
   };
   
   enum Which
   {
      NormalResponse = 0,
      ResponseError = 1,
      RequestError = 2
   };
   
   CPPUNIT_TEST_SUITE(CMultiPingPongTest);
      CPPUNIT_TEST(testMultipleResponses);
      CPPUNIT_TEST(testMultipleResponseErrors);      
      CPPUNIT_TEST(testMultipleRequestErrors);            
      
      CPPUNIT_TEST(testResponseAndResponseError);      
      CPPUNIT_TEST(testResponseErrorAndResponse);

      CPPUNIT_TEST(testResponseAndRequestError);      
      CPPUNIT_TEST(testRequestErrorAndResponse);
      
      CPPUNIT_TEST(testResponseErrorAndRequestError);
      CPPUNIT_TEST(testRequestErrorAndResponseError);      
   CPPUNIT_TEST_SUITE_END();

public:
      
   void testMultipleResponses();
   void testMultipleResponseErrors();
   void testMultipleRequestErrors();
   
   void testResponseAndResponseError();   
   void testResponseErrorAndResponse();   
   
   void testResponseAndRequestError();
   void testRequestErrorAndResponse();   
   
   void testResponseErrorAndRequestError();
   void testRequestErrorAndResponseError();
   
private:   

   void runTest(Mode mode);
};

CPPUNIT_TEST_SUITE_REGISTRATION(CMultiPingPongTest);


// --------------------------------------------------------------------------------


// FIXME add a servicebroker check function somewhere and start automatically if necessary
class CPingPongTestClient : public CPingPongTestDSIProxy
{   
public:

   CPingPongTestClient()
    : CPingPongTestDSIProxy("testping")        
   {
      ::memset(mResponses, 0, sizeof(mResponses));
   }


   void componentConnected()
   {      
      requestPing(L"Hello");
   }

   
   void componentDisconnected()
   {      
      CPPUNIT_ASSERT(engine());
      engine()->stop(0);
   }
   
   void responsePong(const std::wstring& /*message*/)
   {  
      ++mResponses[CMultiPingPongTest::NormalResponse];      
   }

   
   void responseInvalid(PingPongTest::UpdateIdEnum /*id*/)
   {      
      ++mResponses[CMultiPingPongTest::ResponseError];            
   }


   void requestPingFailed(DSI::ResultType /*errType*/)
   {      
      ++mResponses[CMultiPingPongTest::RequestError];       
   }
   
   
   bool check(CMultiPingPongTest::Mode mode) const
   {
      static int expected[] = {
         ResponseBase,
         ResponseErrorBase,
         RequestErrorBase,

         ResponseBase,
         ResponseErrorBase,
      
         ResponseBase,
         RequestErrorBase,
      
         ResponseErrorBase,
         RequestErrorBase,
      };   
      
      return calculate() == expected[mode];
   }
   
private:   

   enum Base
   {
      ResponseBase = 1,         
      ResponseErrorBase = 16,   
      RequestErrorBase = 64    
   };     
   
   int calculate() const
   {       
      return mResponses[0] + mResponses[1] * ResponseErrorBase + mResponses[2] * RequestErrorBase;
   }
   
   char mResponses[3];
};


// -------------------------------------------------------------------------------------


class CPingPongTestServer : public CPingPongTestDSIStub
{
public:
   CPingPongTestServer(CMultiPingPongTest::Mode mode)
    : CPingPongTestDSIStub("testping", false)    
    , mMode(mode)
   {
      // NOOP
   }

   
   void requestPing(const std::wstring& /*message*/)
   {  
      switch(mMode)
      {
      case CMultiPingPongTest::DoubleResponse:
         responsePong(L"Hi");
         responsePong(L"Hi");
         break;
         
      case CMultiPingPongTest::DoubleResponseError:
         sendError(PingPongTest::UPD_ID_responsePong);
         sendError(PingPongTest::UPD_ID_responsePong);
         break;
         
      case CMultiPingPongTest::DoubleRequestError:
         sendError(PingPongTest::UPD_ID_requestPing);
         sendError(PingPongTest::UPD_ID_requestPing);
         break;
         
      case CMultiPingPongTest::ResponseAndResponseError:
         responsePong(L"Hi");
         sendError(PingPongTest::UPD_ID_responsePong);
         break;
         
      case CMultiPingPongTest::ResponseErrorAndResponse:
         sendError(PingPongTest::UPD_ID_responsePong);
         responsePong(L"Hi");
         break;
      
      case CMultiPingPongTest::ResponseAndRequestError:
         responsePong(L"Hi");
         sendError(PingPongTest::UPD_ID_requestPing);
         break;
         
      case CMultiPingPongTest::RequestErrorAndResponse:
         sendError(PingPongTest::UPD_ID_requestPing);
         responsePong(L"Hi");
         break;
      
      case CMultiPingPongTest::ResponseErrorAndRequestError:
         sendError(PingPongTest::UPD_ID_responsePong);
         sendError(PingPongTest::UPD_ID_requestPing);
         break;
         
      case CMultiPingPongTest::RequestErrorAndResponseError:
         sendError(PingPongTest::UPD_ID_requestPing);
         sendError(PingPongTest::UPD_ID_responsePong);
         break;      
         
      default:
         CPPUNIT_FAIL("Unknown mode");
         break;
      }

      engine()->remove(*this);
   }   
   
private:
   CMultiPingPongTest::Mode mMode;
};


// -------------------------------------------------------------------------------------


void CMultiPingPongTest::runTest(Mode mode)
{
   DSI::CCommEngine engine;
   
   CPingPongTestServer serv(mode);
   engine.add(serv);
   
   CPingPongTestClient clnt;
   engine.add(clnt);
   
   CPPUNIT_ASSERT(engine.run() == 0);   
   CPPUNIT_ASSERT(clnt.check(mode));
}


void CMultiPingPongTest::testMultipleResponses()
{
   runTest(DoubleResponse);
}


void CMultiPingPongTest::testResponseAndResponseError()
{
   runTest(ResponseAndResponseError);
}


void CMultiPingPongTest::testResponseErrorAndResponse()
{
   runTest(ResponseErrorAndResponse);
}


void CMultiPingPongTest::testRequestErrorAndResponse()
{
   runTest(RequestErrorAndResponse);
}


void CMultiPingPongTest::testResponseAndRequestError()
{
   runTest(ResponseAndRequestError);
}


void CMultiPingPongTest::testMultipleResponseErrors()
{
   runTest(DoubleResponseError);
}


void CMultiPingPongTest::testMultipleRequestErrors()
{
   runTest(DoubleRequestError);
}


void CMultiPingPongTest::testResponseErrorAndRequestError()
{
   runTest(ResponseErrorAndRequestError);
}


void CMultiPingPongTest::testRequestErrorAndResponseError()
{
   runTest(RequestErrorAndResponseError);
}