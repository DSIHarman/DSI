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

#include "CErrorEnumTestDSIProxy.hpp"
#include "CErrorEnumTestDSIStub.hpp"
#include "CServicebroker.hpp"


/**
 * This test verifies if the code generator generates appropriate code when
 * an error enum is added to the interface (that is an enum named 'Error').
 */
class CErrorEnumTest : public CppUnit::TestFixture
{   
public:      

   enum Mode
   {
      RequestError,
      ResponseError
   };
   
   CPPUNIT_TEST_SUITE(CErrorEnumTest);
      CPPUNIT_TEST(testRequestError);      
      CPPUNIT_TEST(testResponseError);      
   CPPUNIT_TEST_SUITE_END();

public:
      
   void testRequestError();      
   void testResponseError();      
   
private:
   void runTest(Mode m);
};

CPPUNIT_TEST_SUITE_REGISTRATION(CErrorEnumTest);


// --------------------------------------------------------------------------------


class CErrorEnumTestClient : public CErrorEnumTestDSIProxy
{   
public:

   CErrorEnumTestClient(CErrorEnumTest::Mode mode)
    : CErrorEnumTestDSIProxy("attributes")        
    , mMode(mode)    
   {
      // NOOP
   }

   
   void componentConnected()
   {      
      requestGenerateError();
   }
   
   
   void requestGenerateErrorFailed(DSI::ResultType errType)
   {
      CPPUNIT_ASSERT(errType == DSI::RESULT_REQUEST_ERROR);
      CPPUNIT_ASSERT(getLastError() == ErrorEnumTest::ErrorOne);
   }
   
   
   void responseInvalid(ErrorEnumTest::UpdateIdEnum id)
   {
      CPPUNIT_ASSERT(id == ErrorEnumTest::UPD_ID_responseErrorGenerated);
      CPPUNIT_ASSERT(getLastError() == ErrorEnumTest::ErrorTwo);
   }
   
   
   void componentDisconnected()
   {                  
      CPPUNIT_ASSERT(engine());
      engine()->stop(0);           
   }  
   
private:
   CErrorEnumTest::Mode mMode;   
};


// -------------------------------------------------------------------------------------


class CErrorEnumTestServer : public CErrorEnumTestDSIStub
{
public:
   CErrorEnumTestServer(CErrorEnumTest::Mode mode)
    : CErrorEnumTestDSIStub("attributes", false)  
    , mMode(mode)    
   {
      // NOOP
   }   
   
   
   ~CErrorEnumTestServer()
   {     
      // NOOP
   }

   
   void requestGenerateError()
   {
      if (mMode == CErrorEnumTest::RequestError)
      {
         sendError(ErrorEnumTest::UPD_ID_requestGenerateError, ErrorEnumTest::ErrorOne);
      }
      else
      {
         sendError(ErrorEnumTest::UPD_ID_responseErrorGenerated, ErrorEnumTest::ErrorTwo);
      }
      
      CPPUNIT_ASSERT(engine());
      CPPUNIT_ASSERT(engine()->remove(*this));
   }
   
private:
   CErrorEnumTest::Mode mMode;   
};


// -------------------------------------------------------------------------------------


void CErrorEnumTest::runTest(Mode m)
{
   DSI::CCommEngine engine;
   
   CErrorEnumTestServer serv(m);
   engine.add(serv);
   
   CErrorEnumTestClient clnt(m);
   engine.add(clnt);
   
   CPPUNIT_ASSERT(engine.run() == 0);   
}


void CErrorEnumTest::testRequestError()
{
   runTest(RequestError);
}


void CErrorEnumTest::testResponseError()
{
   runTest(ResponseError);
}
