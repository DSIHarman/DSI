/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "dsi/CCommEngine.hpp"
#include "dsi/Log.hpp"

#include "CDSIBenchmarkDSIProxy.hpp"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <cstdio>
#include <cstdarg>

#include <time.h>


/*
 * There are two specifications for the benchmark the Fraunhofer and a newest version from BMW. The benchmark is conform
 * with the BMW specs which try to measure the creation time, too.
 *
 */
#define PING_COUNT 70000


using namespace DSI;


TRC_SCOPE_DEF(imp_sys_dsi_test, client, debug);

namespace /*anonymous*/
{
   bool sLogging = true;
}


static
unsigned int current_time_ms()
{
   struct timespec current_time ;
   clock_gettime(CLOCK_MONOTONIC, &current_time);

   return (current_time.tv_sec * 1000) + (current_time.tv_nsec / 1000000) ;
}


void log(const char* format, ...)
{
   if (sLogging)
   {
      va_list args;
      va_start(args, format);

      ::vprintf(format, args);
   }
}


// ---------------------------------------------------------------------------------


class CDSIBenchmarkDSIProxyImpl : public CDSIBenchmarkDSIProxy
{
public:
   CDSIBenchmarkDSIProxyImpl(CCommEngine& commEngine, int calls = 1)
    : CDSIBenchmarkDSIProxy("GENIVI")
    , mCommEngine(commEngine)
    , mCalls(calls)
    , mCounter(0u)
    , mFactor(0u)
    , mStartTime(0u)
    , mCreationTime(0u)
    , mCreationAndTransportTime(0u)
   {
      mPodStruct.anInt32 = 1;
      mPodStruct.aDouble = 2;
      mPodStruct.anotherDouble = 3;
      mPodStruct.aString = "01234567890123456789";
      // NOOP
   }


   void componentConnected()
   {
      TRC_SCOPE_DEF(imp_sys_dsi_test, client, debug);

      log("Client connected to server, starting sending messages in clone mode\n");

      mCreationTime = 0u;
      mCreationAndTransportTime = 0u;
      mStartTime = current_time_ms();
      requestCallNoArg();
   }


   void componentDisconnected()
   {
      log("Client and server are disconnected\n");
      mCommEngine.stop(0);
   }


   void responseCallNoArg()
   {
      const unsigned int currentTime = current_time_ms();

      mCreationAndTransportTime += currentTime - mStartTime;
      if (mCounter++ < mCalls)
      {
         //no creation time
         mStartTime = currentTime;
         requestCallNoArg();
      }
      else
      {
         snapshot();
         mFactor = 1u;
         mCounter = 0u;
         mCreationTime = 0u;
         mCreationAndTransportTime = 0u;
         mStartTime = current_time_ms();
         //first time send the prepared struct
         createAndSend(mPodStruct);
      }
   }


   void responseCallStruct(const DSIBenchmark::PODStruct &param)
   {
      const unsigned int currentTime = current_time_ms();

      mCreationAndTransportTime += currentTime - mStartTime;
      if (mCounter++ < mCalls)
      {
         mStartTime = currentTime;
         //use the data that we got from the server
         createAndSend(param);
      }
      else
      {
         snapshot();
         mFactor *= 10u;
         mCounter = 0u;
         mCreationTime = 0u;
         mCreationAndTransportTime = 0u;
         mStartTime = current_time_ms();
         //first time send the prepared vector
         createAndSend(mPodStructVector);
      }
   }


   void responseCallStructVector(const DSIBenchmark::PODStructVector &param)
   {
      const unsigned int currentTime = current_time_ms();

      mCreationAndTransportTime += currentTime - mStartTime;
      if (mCounter++ < mCalls)
      {
         mStartTime = currentTime;
         //use the data that we got from the server
         createAndSend(param);
      }
      else
      {
         snapshot();
         mCounter = 0u;
         mCreationTime = 0u;
         mCreationAndTransportTime = 0u;
         if (mFactor < 1000u)
         {
            mFactor *= 10u;
            mStartTime = current_time_ms();
            //use the data that we got from the server
            createAndSend(param);
         }
         else
         {
            //system("ps axv");
            requestShutdown();
            mCommEngine.remove(*this);
         }
      }
   }

   void responseInvalid( DSIBenchmark::UpdateIdEnum id )
   {
      log("Invalid response for %d\n", id);
   }


   void requestCallNoArgFailed( DSI::ResultType errType )
   {
      assert(errType == DSI::RESULT_REQUEST_ERROR);

      log( "Request 'CallNoArg' failed\n");
   }


   void requestCallStructFailed( DSI::ResultType errType )
   {
      assert(errType == DSI::RESULT_REQUEST_ERROR);

      log("Request 'CallStruct' failed\n");
   }


   void requestCallStructVector( DSI::ResultType errType )
   {
      assert(errType == DSI::RESULT_REQUEST_ERROR);

      log("Request 'CallStructVector' failed\n");
   }


   void snapshot()
   {
      if (mCreationAndTransportTime)
      {
         const unsigned int avg_msg_s = (mCalls * 1000 * 2) / mCreationAndTransportTime; //a ping-pong loop has 2 messages

         /*
           Note: creation time can not be measured accurately with the benchmark.

           We measure the time to create messages on the client side only, therefore we consider the server time equal
           and double it to get total creation time on both sides.
         */
         log("%4u x struct(s): %u calls, total time %u ms, creation time %u, transport time %u, %u msgs/s\n",
             mFactor, mCalls, mCreationAndTransportTime, 2 * mCreationTime, mCreationAndTransportTime - 2 * mCreationTime, avg_msg_s);
      }
      else
      {
         log("%4u x struct(s): Not enough time measurement resolution\n", mFactor);
      }
   }


   void createAndSend(const DSIBenchmark::PODStructVector &param)
   {
      DSIBenchmark::PODStructVector localParameter(mFactor);

      while (localParameter.size() < mFactor)
      {
         localParameter.push_back(param[0]);
      }
      mCreationTime += current_time_ms() - mStartTime;

      CDSIBenchmarkDSIProxy::requestCallStructVector(localParameter);
   }


   void createAndSend(const DSIBenchmark::PODStruct &param)
   {
      /*
        Create a new message and fill it out, the copy constructor will do that for us.
      */
      DSIBenchmark::PODStruct localParameter(param);

      mCreationTime += current_time_ms() - mStartTime;
      requestCallStruct(localParameter);
   }


private:
   /// The DSI connection to the server
   CCommEngine &mCommEngine;

   /// Number of pings to send per cycle to the server
   unsigned int mCalls;

   /// Number of pings sent in the current cycle to the server
   unsigned int mCounter;

   /// Defines how many structs must be sent in this cycle
   unsigned int mFactor;

   /// Time to measure one ping-pong
   unsigned int mStartTime;

   /// Total time needed to create the messages to be sent to the server per cycle
   unsigned int mCreationTime;

   /// Total time to create and transport the messages to server and back per cycle
   unsigned int mCreationAndTransportTime;

   /// The 40 bytes struct to be sent
   DSIBenchmark::PODStruct mPodStruct;

   /// A vector to be used when sending more structs in a call
   DSIBenchmark::PODStructVector mPodStructVector;
};


// -----------------------------------------------------------------------------------


int main(int argc, char** argv)
{
   TRC_SCOPE(imp_sys_dsi_test, client, main);
   log("Ping client application started\n");

   CCommEngine commEngine;
   Log::setDevice(Log::Console);

   int calls = PING_COUNT;    // take this as default count of calls
   if (argc >= 2)
   {
      for (int i = 1; i < argc; i++)
      {
         if (!strncmp(argv[i], "--pings=", 8))
         {
            calls = atoi(argv[1] + 8);
            assert(calls > 0);
         }
         else if (strncmp(argv[i], "--silent", 3))
         {
            sLogging = false;
         }
      }
   }

   CDSIBenchmarkDSIProxyImpl proxy(commEngine, calls);
   commEngine.add(proxy);

   commEngine.run();

   return 0;
}
