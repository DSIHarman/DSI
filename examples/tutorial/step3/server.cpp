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
#include <sstream>
#include <list>


TRC_SCOPE_DEF(examples_tutorial_step3, Server, main);


using namespace DSI;

class CCalculatorDSIStubImpl : public CCalculatorDSIStub
{
public:

   CCalculatorDSIStubImpl(CCommEngine& commEngine)
      : CCalculatorDSIStub("calculator", ::getenv("DSI_FORCE_TCP") ? true : false/*enable tcpip*/)
      , mCommEngine(commEngine)
      , mHistory()
   {
      setCurrentValue(0);
   }

   void requestAdd(double op1, double op2)
   {
      std::cout << "server: received requestAdd(" << op1 << ", " << op2 << ")" << std::endl;

      addHistoryEntry(toWString("add(") + toWString(op1) + toWString(", ") + toWString(op2) + toWString(")"));
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

   void informationHistoryChanged(uint32_t size)
   {
      std::cout << "server: sending informationHistoryChanged(" << size << ")" << std::endl;
      CCalculatorDSIStub::informationHistoryChanged(size);
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

   template <typename T>
   std::wstring toWString(T t)
   {
      std::ostringstream oStream;

      oStream << t;
      std::string str(oStream.str());
      std::wstring str2(str.length(), L' '); // Make room for characters

      std::copy(str.begin(), str.end(), str2.begin());

      return str2;
   }

   void addHistoryEntry(std::wstring command)
   {
      time_t rawtime;
      struct tm *timeinfo;

      time(&rawtime);
      timeinfo = localtime(&rawtime);
      mHistory.push_front(Calculator::THistoryEntry(toWString(asctime(timeinfo)), command));
      informationHistoryChanged(mHistory.size());
   }

   CCommEngine& mCommEngine;

   ///keeps the history of requests received
   std::list<Calculator::THistoryEntry> mHistory;
};


// -----------------------------------------------------------------------------------


int main(int argc, char** argv)
{
   TRC_SCOPE(examples_tutorial_step3, Server, main);
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

   DBG_MSG(("server: Calculator application started"));


   CCommEngine commEngine;
   CCalculatorDSIStubImpl stub(commEngine);
   commEngine.add(stub);

   commEngine.run();

   return 0;
}
