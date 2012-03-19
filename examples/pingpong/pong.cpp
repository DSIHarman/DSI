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
#include "dsi/CStdoutTracer.hpp"

#include "CPingPongDSIStub.hpp"

#include <cstdlib>
#include <iostream>


TRC_SCOPE_DEF(imp_sys_dsi_test, Pong, main);

// globals
static int expected = 0;

using namespace DSI;

class CPingPongDSIStubImpl : public CPingPongDSIStub
{
public:

   CPingPongDSIStubImpl(CCommEngine& commEngine)
    : CPingPongDSIStub("ping", ::getenv("DSI_FORCE_TCP") ? true : false/*enable tcpip*/)
    , mCommEngine(commEngine)
   {
      // NOOP
   }


   void requestPing( const std::wstring& message )
   {
      std::wcout << L"---> Ping " << message << std::endl;

      if (expected == 0)
      {
         responsePong(L"from stub");
      }
      else if (expected == 1)
      {
         sendError(PingPong::UPD_ID_requestPing);
      }
      else
         sendError(PingPong::UPD_ID_responsePong);
   }

private:

   CCommEngine& mCommEngine;
};


// -----------------------------------------------------------------------------------


int main(int argc, char** argv)
{
   TRC_SCOPE(imp_sys_dsi_test, Pong, main);
   DBG_MSG(("Pong server application started"));

   DSI::Trace::CStdoutTracer tracer;
   DSI::Trace::init(tracer);   
   
   for(int i=1; i < argc; ++i)
   {
      if (!strcmp(argv[i], "--error-on-request"))
      {
         expected = 1;
      }
      else if (!strcmp(argv[i], "--error-on-response"))
      {
         expected = 2;
      }
      else if (!strncmp(argv[i], "-v", 2))
      {
         char* ptr = argv[i]+1;

         while(*ptr && *ptr == 'v')
            ++ptr;

         Log::setLevel(ptr - argv[i] - 1);
      }
   }

   CCommEngine commEngine;
   CPingPongDSIStubImpl stub(commEngine);
   commEngine.add(stub);

   commEngine.run();

   return 0;
}
