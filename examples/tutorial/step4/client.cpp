/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
// The client side of the Calculator example.
#include "CCalculatorDSIProxy.hpp"

#include "dsi/CCommEngine.hpp"
#include "dsi/Log.hpp"


#include <cassert>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <cstdio>

using namespace DSI;

TRC_SCOPE_DEF(examples_tutorial_step3, Client, main);

/// The client side of the Calculator example.
class CCalculatorDSIProxyImpl : public CCalculatorDSIProxy
{
public:

   CCalculatorDSIProxyImpl(CCommEngine& commEngine)
      : CCalculatorDSIProxy("calculator")
      , mCommEngine(commEngine)
   {
      // NOOP
   }


   void componentConnected()
   {
      std::cout << "client: connected to server" << std::endl;
      Calculator::CUpdateIdVector updates;

      updates.push_back(Calculator::UPD_ID_informationRisingValue);
      registerRisingValue(updates, 10);

      std::cout << "client: sending requestAdd(5, 4)" << std::endl;
      requestAdd(5, 4);
   }


   void componentDisconnected()
   {
      std::cout << "client: server disconnected" << std::endl;
      mCommEngine.stop(0);
   }


   void responseOperationResult(double result, Calculator::EResult error)
   {
      std::cout << "client: received result=" << result <<  " and error code=" << error << std::endl;

      std::cout << "client: sending requestAdd(5, 5)" << std::endl;
      requestAdd(5, 5);
   }


   void informationRisingValue(int32_t value)
   {
      std::cout << "client: received informationRisingValue(" << value <<  ")" << std::endl;
      mCommEngine.stop(0);
   }


private:
   DSI::CCommEngine& mCommEngine;
};




// -----------------------------------------------------------------------------------


int main(int argc, char** argv)
{
   TRC_SCOPE(examples_tutorial_step3, Client, main);
   Log::setDevice(Log::Console);

   CCommEngine commEngine;

   for(int i=1; i < argc; ++i)
   {
      if (!strncmp(argv[i], "-v", 2))
      {
         char* ptr = argv[i]+1;

         while(*ptr && *ptr == 'v')
            ++ptr;

         Log::setLevel(ptr - argv[i] + 1);
      }
   }
   DBG_MSG(("client: Calculator application started"));


   CCalculatorDSIProxyImpl proxy(commEngine);
   commEngine.add(proxy);

   commEngine.run();

   return 0;
}
