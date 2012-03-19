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

#include "CCalculatorDSIStub.hpp"

#include <cstdlib>
#include <iostream>


TRC_SCOPE_DEF(examples_tutorial_step2, Server, main);


using namespace DSI;

class CCalculatorDSIStubImpl : public CCalculatorDSIStub
{
public:

   CCalculatorDSIStubImpl(CCommEngine& commEngine)
      : CCalculatorDSIStub("calculator", ::getenv("DSI_FORCE_TCP") ? true : false/*enable tcpip*/)
      , mCommEngine(commEngine)
   {
      setCurrentValue(0);
   }

   void requestAdd(double op1, double op2)
   {
      std::cout << "server: received requestAdd(" << op1 << ", " << op2 << ")" << std::endl;
      responseOperationResult(op1 + op2, Calculator::OP_SUCCESS);
   }

   void responseOperationResult(double result, Calculator::EResult error)
   {
      std::cout << "server: sending responseOperationResult(" << result << ")" << std::endl;
      CCalculatorDSIStub::responseOperationResult(result, error);
      setCurrentValue(result);
   }

   void setCurrentValue(double currentValue)
   {
      std::cout << "server: setting currentValue to " << currentValue << std::endl;
      CCalculatorDSIStub::setCurrentValue(currentValue);
   }


   //the rest of the abstract methods are not important for this tutorial step
   void registerRisingValue(int32_t)
   {
      //NOP
   }

   void unregisterRisingValue(int32_t)
   {
      //NOP
   }

   void requestSubtract(double, double)
   {
      //NOP
   }

   void requestChangeBase(Calculator::EBase)
   {
      //NOP
   }

   void requestGetHistory(uint32_t)
   {
      //NOP
   }

   void requestMultiply(double, double)
   {
      //NOP
   }

   void requestDivide(double, double)
   {
      //NOP
   }

private:

   CCommEngine& mCommEngine;
};


// -----------------------------------------------------------------------------------


int main(int argc, char** argv)
{
   TRC_SCOPE(examples_tutorial_step2, Server, main);
   Log::setDevice(Log::Console);
   
   for(int i=1; i < argc; ++i)
   {
      if (!strncmp(argv[i], "-v", 2))
      {
         char* ptr = argv[i]+1;

         while(*ptr && *ptr == 'v')
            ++ptr;

         Log::setLevel(ptr - argv[i] - 1);
      }
   }
   DBG_MSG(("Server application started"));

   CCommEngine commEngine;
   CCalculatorDSIStubImpl stub(commEngine);
   commEngine.add(stub);

   commEngine.run();

   return 0;
}
