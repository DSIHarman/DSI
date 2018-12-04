/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
// Ping is the client side of a pingpong example

#include "dsi/CCommEngine.hpp"
#include "dsi/Log.hpp"

#include "CDSIBenchmarkDSIStub.hpp"

#include <cstdlib>
#include <iostream>

TRC_SCOPE_DEF(imp_sys_dsi_test, Server, main);


using namespace DSI;


class CDSIBenchmarkDSIStubImpl : public CDSIBenchmarkDSIStub
{
public:

   CDSIBenchmarkDSIStubImpl(CCommEngine& commEngine)
      : CDSIBenchmarkDSIStub("GENIVI", false/*enable tcpip*/)
      , mCommEngine(commEngine)
   {
      // NOOP
   }


   void requestCallNoArg()
   {
      responseCallNoArg();
   }

   void requestCallStruct(const DSIBenchmark::PODStruct &param1)
   {
      /*
        Create a new message and fill it out, the copy constructor will do that for us.
       */
      DSIBenchmark::PODStruct localParameter(param1);

      responseCallStruct(localParameter);
   }


   void requestCallStructVector(const DSIBenchmark::PODStructVector &param1)
   {
      /*
        Create a new message and fill it out, the copy constructor will do that for us.
       */
      DSIBenchmark::PODStructVector localParameter(param1);

      responseCallStructVector(localParameter);
   }


   void requestShutdown()
   {
      mCommEngine.remove(*this);
      mCommEngine.stop(0);
   }


private:

   CCommEngine& mCommEngine;
};


// -----------------------------------------------------------------------------------


int main(void)
{
   TRC_SCOPE(imp_sys_dsi_test, Server, main);
   DBG_MSG(("DSIBenchmark server application started"));


   CCommEngine commEngine;
   CDSIBenchmarkDSIStubImpl stub(commEngine);
   commEngine.add(stub);

   commEngine.run();

   return 0;
}
