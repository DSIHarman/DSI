/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
// Ping is the client side of a pingpong example.
// Sends n (->cmdline option) pings with a time delay of 1 second between the pings.

#include "dsi/CCommEngine.hpp"
#include "dsi/Log.hpp"
#include "dsi/CStdoutTracer.hpp"

#include "CPingPongDSIProxy.hpp"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <cstdio>

#define SLEEP_SECS 1

using namespace DSI;

TRC_SCOPE_DEF(imp_sys_dsi_test, Ping, main);

// globals
static int actual = 0;


class CPingPongDSIProxyImpl : public CPingPongDSIProxy
{
   public:

      CPingPongDSIProxyImpl(CCommEngine& commEngine, int calls = 1)
         : CPingPongDSIProxy("ping")
         , mCommEngine(commEngine)
         , mCalls(calls)
      {
         // NOOP
      }


      void componentConnected()
      {
         std::cout << "Proxy connected to server" << std::endl;
         requestPing(L"from proxy");
      }


      void componentDisconnected()
      {
         std::cout << "client and server are disconnected" << std::endl;
         mCommEngine.stop(0);
      }


      void responsePong(const std::wstring& message)
      {
         std::wcout << L"<--- Pong " << message << std::endl;

         if (--mCalls == 0)
         {
            mCommEngine.remove(*this);
         }
         else
         {
            sleep(SLEEP_SECS);
            requestPing(L"from proxy");
         }
      }


      void responseInvalid( PingPong::UpdateIdEnum id )
      {
         actual = 2;

         std::cout << "Invalid response for 'Ping'" << std::endl;
         assert(id == PingPong::UPD_ID_responsePong);

         if (--mCalls == 0)
            mCommEngine.remove(*this);
         else
         {
            sleep(SLEEP_SECS);
            requestPing(L"from proxy");
         }
      }


      void requestPingFailed( DSI::ResultType errType )
      {
         actual = 1;

         assert(errType == DSI::RESULT_REQUEST_ERROR);

         std::cout << "Request 'Ping' failed" << std::endl;

         if (--mCalls == 0)
         {
            mCommEngine.remove(*this);
         }
         else
         {
            sleep(SLEEP_SECS);
            requestPing(L"from proxy");
         }
      }

   private:

      CCommEngine& mCommEngine;
      int mCalls;
};


// -----------------------------------------------------------------------------------


int main(int argc, char** argv)
{
   TRC_SCOPE(imp_sys_dsi_test, Ping, main);
   DBG_MSG(("Ping client application started"));

   DSI::Trace::CStdoutTracer tracer;
   DSI::Trace::init(tracer);
   
   CCommEngine commEngine;

   int calls = 1;    // take this as default count of calls

   for(int i=1; i < argc; ++i)
   {
      if (!strncmp(argv[i], "--pings=", 8))
      {
         calls = atoi(argv[i] + 8);
         assert(calls > 0);
      }
      if (!strncmp(argv[i], "-v", 2))
      {
         char* ptr = argv[i]+1;

         while(*ptr && *ptr == 'v')
            ++ptr;

         printf("setting loglevel %d\n", ptr - argv[i] - 1);
         Log::setLevel(ptr - argv[i] + 1);
      }
   }

   CPingPongDSIProxyImpl proxy(commEngine, calls);
   commEngine.add(proxy);

   commEngine.run();

   int expected = 0;
   if (argc >= 2)
   {
      if (!strcmp(argv[1], "--error-on-request"))
      {
         expected = 1;
      }
      else if (!strcmp(argv[1], "--error-on-response"))
      {
         expected = 2;
      }
   }
   assert(expected == actual);

   return 0;
}
