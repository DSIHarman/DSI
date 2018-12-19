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


TRC_SCOPE_DEF(examples_tutorial_step4, Server, main);


using namespace DSI;

class CCalculatorDSIStubImpl : public CCalculatorDSIStub
{
public:

   CCalculatorDSIStubImpl(CCommEngine& commEngine)
      : CCalculatorDSIStub("calculator", ::getenv("DSI_FORCE_TCP") ? true : false/*enable tcpip*/)
      , mCommEngine(commEngine)
      , mClients()
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
      checkWatermarks();
   }

   void informationRisingValue(int32_t value)
   {
      std::cout << "server: sending informationRisingValue(" << value << ")" << std::endl;
      CCalculatorDSIStub::informationRisingValue(value);
   }

   void registerRisingValue(int32_t highWatermark)
   {
      std::cout << "server: received registerRisingValue(" << highWatermark << ")" << std::endl;
      mClients.insert(std::pair<int32_t,int32_t>(registerCurrentSession(), highWatermark));
      checkWatermarks();
   }

   void unregisterRisingValue(int32_t sessionID)
   {
      std::cout << "server: received unregisterRisingValue(" << sessionID << ")" << std::endl;
      //client removed the highWatermark register monitoring request
      mClients.erase(mClients.find(sessionID));
   }

   /*
    * Checks if the registered highWatermarks set by the clients are reached. In positive case send the information to
    * the corresponding clients.
   */
   void checkWatermarks()
   {
      TClientsMapIt it;

      for(it = mClients.begin(); it != mClients.end(); it++)
      {
         if ((*it).second <= getCurrentValue())
         {
            addActiveSession((*it).first);            
         }
      }
      
      informationRisingValue(getCurrentValue());
      clearActiveSessions();
   }


   //the rest of the abstract methods are not important for this tutorial step
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


   typedef std::map<int32_t, int32_t> TClientsMap;
   typedef std::map<int32_t, int32_t>::iterator TClientsMapIt;

   ///keeps all watermarks set by the clients: the key is the sessionID and the value is the watermark
   TClientsMap mClients;
};


// -----------------------------------------------------------------------------------


int main(int argc, char** argv)
{
   TRC_SCOPE(examples_tutorial_step4, Server, main);
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
