/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "MasterAdapter.hpp"

#include "Notifier.hpp"
#include "Servicebroker.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "config.h"
#include "Log.hpp"

using namespace DSI;

// the servicebroker does not need this lock since there is only the master adapter worker thread who
// calls functions from this module
#define SB_NO_CRITICAL_SECTION 1
#   include "clientIo.cpp"
#undef SB_NO_CRITICAL_SECTION


struct MasterConnectorSocket : public Private::CNonCopyable
{
  MasterConnectorSocket(const char* address)
    : mConnected(false)
    , mEndpoint(address)
  {
    assert(mEndpoint.getIP() && mEndpoint.getPort());
    Log::message( 3, "Socket Master Connection: %s", address);
  }

  bool connect()
  {
    if (mSock.open() == io::ok)
    {
      // set the send timeout to 5 seconds in order to react in-time on a network failure
      // which otherwise would be noticed after minutes in respect of tcp transmission timeouts
      (void)mSock.setSocketOption(SocketSendTimeoutOption(SB_MASTERADAPTER_SENDTIMEO));
      (void)mSock.setSocketOption(SocketReceiveTimeoutOption(SB_MASTERADAPTER_RECVTIMEO));

      io::error_code ec = io::unset;

      do
      {
        ec = mSock.connect(mEndpoint);
      }
      while(ec == io::interrupted);

      if (ec == io::ok)
      {
        mConnected = true;
        (void)mSock.setSocketOption(SocketTcpNoDelayOption<true>());

        if (sendAuthPackage(mSock.fd(), 0/*=no credentials via TCP sockets*/) != 0)
          disconnect();
      }
      else
        (void)mSock.close();
    }

    return mSock.is_open();
  }

  inline
  bool isConnected() const
  {
    return mConnected;
  }

  inline
  void disconnect()
  {
    (void)mSock.close();
    mConnected = false;
  }

  inline
  uint32_t getMasterAddress() const
  {
    return htonl(mEndpoint.getIP());
  }

  inline
  bool sendPing()
  {
    return sendAndReceive(mSock.fd(), DCMD_FND_MASTER_PING, nullptr, 0, 0, nullptr) == 0;
  }

  inline
  bool sendIdPing()
  {
    union SFNDMasterPingIdArg data;
    data.i.sbVersion.majorVersion = DSI_SERVICEBROKER_VERSION_MAJOR;
    data.i.sbVersion.minorVersion = DSI_SERVICEBROKER_VERSION_MINOR;
    data.i.id = Servicebroker::getInstance().id();
    data.i.reserved = 0;

    return sendAndReceive(mSock.fd(), DCMD_FND_MASTER_PING_ID, &data, sizeof(data), 0, nullptr) == 0;
  }

  inline
  bool execute(Job& job)
  {
    job.status = sendAndReceive(mSock.fd(), job.dcmd, job.arg, job.sbytes, job.rbytes, &job.ret_val);
    bool rc = job.status == 0;

    // correction for GetInterfaceList and MatchInterfaceList
    if (job.status == 0)
    {
      if (job.ret_val < 0)
      {
        job.ret_val = -job.ret_val;
      }
      else if (job.ret_val == FNDBadArgument)
        job.status = EINVAL;
    }
    else
    {
      int error = errno;
      Log::error("Master TCP communication failed: errno=%d", error);
    }

    return rc;
  }

private:

  IPv4::StreamSocket mSock;
  bool mConnected;

  IPv4::Endpoint mEndpoint;
};


// ------------------------------------------------------------------------------------------


MasterAdapter::MasterAdapter(const char* address)
  : mConnector(new MasterConnectorSocket(address))
  , mAddress(address)
  , mTrigger(nullptr)
{
  // NOOP
}


MasterAdapter::~MasterAdapter()
{
  // NOOP
}


bool MasterAdapter::connect()
{
  return mConnector->connect();
}


bool MasterAdapter::isConnected() const
{
  return mConnector->isConnected();
}


uint32_t MasterAdapter::getMasterAddress() const
{
  return mConnector->getMasterAddress();
}


void MasterAdapter::disconnect()
{
  mConnector->disconnect();
}


bool MasterAdapter::sendPing()
{
  return mConnector->sendPing();
}


bool MasterAdapter::sendIdPing()
{
  return mConnector->sendIdPing();
}


bool MasterAdapter::execute(Job& job, Notifier& notifier)
{
  bool rc = mConnector->execute(job);
  notifier.jobFinished(job);
  return rc;
}


void MasterAdapter::removePending(Notifier& notifier)
{
  /*
   * removing all jobs from the queue and returning an error.
   * error handling is don in the servicebroker thread
   */
  Job* job = mQueue.pop();
  while( job )
  {
    job->status = EIO;
    notifier.jobFinished(*job);
    job = mQueue.pop();
  }
}


bool MasterAdapter::executePending(Notifier& notifier)
{
  bool rc = true;

  /*
   * executing all jobs in the queue and sending them back to the
   * servicebroker
   */
  Job* job = mQueue.pop();
  while(job)
  {
    rc = execute(*job, notifier);
    if (!rc)
      break;

    job = mQueue.pop();
  }

  return rc;
}


