/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "ServicebrokerServer.hpp"

#include <ctype.h>   // tolower

#include "Log.hpp"
#include "util.hpp"
#include "MasterAdapter.hpp"
#include "Servicebroker.hpp"
#include "SocketMessageContext.hpp"
#include "TCPMasterNotificationReceiver.hpp"

#ifndef UNIX_PATH_MAX
#   define UNIX_PATH_MAX 108
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>

using namespace DSI;

//maximum network interfaces to bind
#define MAX_IPS_TO_BIND 5

namespace /*anonymous*/
{

  unsigned short parseIPs(const char* dottedIps, int *ips, unsigned short ips_size)
  {
    struct in_addr tmpAddr;
    unsigned short i(0);

    if (!dottedIps || !ips || !ips_size)
    {
      return 0;
    }
    char *tmpIps = new char[strlen(dottedIps)];
    char *token;

    strcpy(tmpIps, dottedIps);
    token = strtok(tmpIps, ",");
    while (token)
    {
      if (inet_pton(AF_INET, token, &tmpAddr) != 1)
      {
        Log::error("Invalid IP address to bind: %s", token);
      }
      else
      {
        ips[i++] = ntohl(tmpAddr.s_addr);
      }
      token = strtok(nullptr, ",");
    }
    delete []tmpIps;
    return i;
  }


  bool initSocket(IPv4::Acceptor& sock, int port, int32_t ip = 0)
  {
    bool result = true;

    port = getRealPort(port);

    if (!sock.is_open())
      result = sock.open() == io::ok;

    if (result)
    {
      (void)sock.setSocketOption(SocketReuseAddressOption<true>());

      if (ip)
      {
        result = sock.bind(IPv4::Endpoint(ip, port)) == io::ok
          && sock.listen() == io::ok;
      }
      else
      {
        result = sock.bind(IPv4::Endpoint(port)) == io::ok
          && sock.listen() == io::ok;
      }

      if (!result)
        Log::error("Cannot bind to port %d.", port);
    }

    if (!result)
      Log::error("Cannot (re)initialize acceptor socket on port %d.", port);

    return result;
  }

}   // namespace /*anonymous*/


ServicebrokerClientBase::ServicebrokerClientBase(ServicebrokerServer& server, uint32_t nodeAddress, uint32_t localAddress)
  : mContext(mData, nodeAddress, localAddress, self())
  , mServer(server)
  , mState(State_READING_HEADER)
{
  // NOOP
}


ServicebrokerClientBase::~ServicebrokerClientBase()
{
  mServer.removeClient(*this);

  Log::warning( 3, ">IOCLOSE OCB [%d]", mContext.getId() );
  mData.clear(mContext.isSlave());
}


void ServicebrokerClientBase::eval(sb_request_frame_t& f)
{
  // default to unknown unhandled command
  INIT_RESPONSE_FRAME(&mWriteBuffer.response(), 0, (int)FNDUnknownCommand);

  SocketMessageContext msgctx(mContext, mReadBuffer, mWriteBuffer);
  int cmd = f.fr_cmd;

  bool found = Servicebroker::getInstance().dispatch(f.fr_cmd, msgctx, mData);

  // extra handling for this initial master ping -> throw away any old connection
  // with same id (-> is probably dead) and add the new one.
  // Also, this can only be done with an explicit ID given, otherwise we just cannot detect client death.
  if (f.fr_cmd == DCMD_FND_MASTER_PING_ID && mContext.getData().getExtendedID() != 0)
  {
    mServer.removeClient(mContext.getData().getExtendedID());
    mServer.addClient(*this);
  }

  if (found && msgctx.responseState() == MessageContext::State_DEFERRED)
  {
    // keep the context object alive since operation is not finished for this client, no reponse is sent back.
    mState = State_DEFERRED;
  }
  else
  {
    if (!found)
      Log::message(1, "Received unknown request '%d'\n", cmd);

    mReadBuffer.reset();   // free data - if possible and necessary
    mWriteBuffer.set(0);   // seek buffer to beginning

    evaluateWriteResult();
  }
}


bool ServicebrokerClientBase::handleRead(size_t amount, io::error_code err)
{
  if (err == io::ok)
  {
    if (mState == State_READING_HEADER)
    {
      mReadBuffer.bump(amount);

      if (mReadBuffer.pos() >= sizeof(sb_request_frame_t))
      {
        if (mReadBuffer.request().fr_envelope.fr_magic == SB_FRAME_MAGIC)
        {
          mReadBuffer.reserve(mReadBuffer.request().fr_envelope.fr_size + sizeof(sb_request_frame_t));
          mState = State_READING_BODY;
        }
        else
        {
          Log::error("Invalid request frame magic detected. Closing connection.");
          delete this;

          return false;
        }
      }
    }
    else if (mState == State_READING_BODY)
    {
      mReadBuffer.bump(amount);
    }

    // request already complete?
    if (mReadBuffer.pos() == mReadBuffer.request().fr_envelope.fr_size + sizeof(sb_request_frame_t))
    {
      eval(mReadBuffer.request());
    }
    else
      arm();   // request data not yet complete
  }
  else
    delete this;

  return false;
}


bool ServicebrokerClientBase::handleWrite(size_t amount, io::error_code err)
{
  if (err == io::ok)
  {
    mWriteBuffer.bump(amount);

    if (mWriteBuffer.pos() == mWriteBuffer.response().fr_envelope.fr_size + sizeof(sb_response_frame_t))
    {
      mState = State_READING_HEADER;
      mWriteBuffer.reset();
    }

    arm();
  }
  else
    delete this;

  return false;
}


void ServicebrokerClientBase::sendDeferredResponse(int status, int retval, void* data, size_t len)
{
  assert(mState == State_DEFERRED);

  if (status != 0)
  {
    INIT_RESPONSE_FRAME(&mWriteBuffer.response(), 0, (int)FNDInternalError);
  }
  else
  {
    if (retval <= (int)FNDOK)
    {
      (void)mWriteBuffer.write(data, len, sizeof(sb_response_frame_t));
      INIT_RESPONSE_FRAME(&mWriteBuffer.response(), len, retval);
    }
    else
    {
      INIT_RESPONSE_FRAME(&mWriteBuffer.response(), 0, retval);
    }
  }

  mReadBuffer.reset();
  mWriteBuffer.set(0);

  mState = State_WRITING;
  arm();
}


// -------------------------------------------------------------------------------------------------------


#ifdef HAVE_HTTP_SERVICE


const char* HTTPClient::parseRequestLine()
{
  if (strnlen(buf_, sizeof(buf_)-1) > 4)
  {
    // cut out the first line
    char* ptr = buf_;

    while(*ptr != '\r' && *ptr != 'n')
      ++ptr;

    *ptr = '\0';

    // parse uri from first line - GET <uri> HTTP/1.1
    ptr = buf_;
    while(*ptr != ' ' && *ptr != '\0')
      ++ptr;

    if (*ptr == ' ')
      ++ptr;

    const char* uri = ptr;

    while(*ptr != ' ' && *ptr != '\0')
      ++ptr;

    *ptr = '\0';

    return uri;
  }

  return nullptr;
}


int HTTPClient::parseContentLength()
{
  char buf[sizeof(buf_)];

  ::memcpy(buf, buf_, sizeof(buf));
  buf[sizeof(buf)-1] = '\0';

  char* ptr = buf;

  // convert lowercase
  while(*ptr != '\0')
  {
    *ptr = ::tolower(*ptr);
    ++ptr;
  }

  if ((ptr = ::strstr(buf_, "content-length")))
  {
    ptr += 15;  // skip field-name

    while(*ptr == ':' || *ptr ==' ')  // skip until field value
      ++ptr;

    return ::atoi(ptr);
  }

  return -1;
}


HTTPClient::HTTPClient(const IPv4::StreamSocket& sock)
  : mSock(sock)
  , body_begin_(nullptr)
  , total_(0)
{
  ::memset(buf_, 0, sizeof(buf_));
}


void HTTPClient::start()
{
  mSock.async_read_some(buf_, sizeof(buf_), bind3(&HTTPClient::handleRead, std::tr1::ref(*this),
                                  std::tr1::placeholders::_1, std::tr1::placeholders::_2));
}


bool HTTPClient::handleRead(size_t len, io::error_code err)
{
  if (err == io::ok)
  {
    total_ += len;
    bool rearmRead = total_ < static_cast<int>(sizeof(buf_));

    char* ptr;
    if ((ptr = strstr(buf_, "\r\n\r\n")))   // header complete?
    {
      body_begin_ = ptr + 2;

      rearmRead = false;

      if (buf_[0] == 'P'/*OST*/)
      {
        int len = parseContentLength();

        // content complete?
        if (total_ < body_begin_ - buf_ + len && total_ < static_cast<int>(sizeof(buf_)))
          rearmRead = true;
      }
    }

    if (rearmRead)
    {
      mSock.async_read_some(buf_ + total_, sizeof(buf_) - total_,
                     bind3(&HTTPClient::handleRead, std::tr1::ref(*this), std::tr1::placeholders::_1,
                         std::tr1::placeholders::_2));
    }
    else
      eval();
  }
  else
    delete this;

  return false;
}


void HTTPClient::eval()
{
  std::ostringstream body;

  if (buf_[0] == 'P'/*OST*/)
  {
    if (body_begin_ != nullptr && total_ < static_cast<int32_t>(sizeof(buf_)))   // avoid any request overflows...
    {
      // skip whitespace if any
      while(::isspace(*body_begin_))
        ++body_begin_;

      // parse name/value pairs
      char* token = strtok(body_begin_, "&");
      while(token != nullptr)
      {
        // parse name=value pair
        setup(token);

        // next token
        token = strtok(nullptr, "&");
      }

      body << "OK";
    }
    else
      body << "FAIL";
  }
  else
  {
    // assume GET
    const char* uri = parseRequestLine();

    if (uri && *uri != '\0')
    {
      switch(uri[1])
      {
        case 's':
          Servicebroker::getInstance().dumpServers(body);
          break;

        case 'a':
          Servicebroker::getInstance().dumpClients(body);
          break;

        case 'c':
          Servicebroker::getInstance().dumpServerConnectNotifications(body);
          break;

        case 'd':
          switch(uri[2])
          {
            case '\0':
              Servicebroker::getInstance().dumpServerDisconnectNotifications(body);
              break;

            default:
              Servicebroker::getInstance().dumpClientDetachNotifications(body);
              break;
          }
          break;

        case 'f'/*avico'*/:
          break;

        default:
          Servicebroker::getInstance().dumpStats(body);
          break;
      }
    }
  }

  mBody = body.str();

  std::ostringstream header;
  header
    << "HTTP/1.1 200 OK\r\n"
    << "Content-type: text/plain\r\n"
    << "Content-transfer-encoding: 7-bit\r\n"
    << "Content-length: " << mBody.size() << "\r\n"
    << "Connection: close\r\n"
    << "\r\n";

  mHeader = header.str();

  mState = State_SENDHEADER;
  arm();   // ...to send header
}


bool HTTPClient::handleWrite(size_t /*amount*/, io::error_code err)
{
  if (err == io::ok)
  {
    if (mState == State_SENDHEADER)
    {
      mState = State_SENDBODY;
      arm();  // ...to send body
    }
    else
      delete this;
  }
  else
    delete this;

  return false;
}


void HTTPClient::arm()
{
  if (mState == State_SENDHEADER)
  {
    mSock.async_write_all(mHeader.c_str(), mHeader.size(), bind3(&HTTPClient::handleWrite, std::tr1::ref(*this),
                                             std::tr1::placeholders::_1, std::tr1::placeholders::_2));
  }
  else
  {
    mSock.async_write_all(mBody.c_str(), mBody.size(), bind3(&HTTPClient::handleWrite, std::tr1::ref(*this),
                                          std::tr1::placeholders::_1, std::tr1::placeholders::_2));
  }
}

#endif   // HAVE_HTTP_SERVICE


// -------------------------------------------------------------------------------------------------------


struct ExtendedIdPredicate
{
public:

  ExtendedIdPredicate(int32_t extendedID)
    : mExtendedID(extendedID)
  {
    // NOOP
  }

  template<typename ElementT>
  bool operator()(const ElementT& e) const
  {
    bool rc = (e.getContext().getData().getExtendedID() == mExtendedID);

    if (rc)
      Log::warning(2, "Removing possibly dangling client OCB [%d] with id=%d", e.getContext().getId(), mExtendedID);

    return rc;
  }

  int32_t mExtendedID;
};


struct PointerComparePredicate
{
  PointerComparePredicate(const void* ptr)
    : mPtr(ptr)
  {
    // NOOP
  }

  template<typename ElementT>
  bool operator()(const ElementT& e) const
  {
    return &e == mPtr;
  }

  const void* mPtr;
};


ServicebrokerServer::ServicebrokerServer()
  : HTTPMixin(mService)
  , NotificationMixin(mService)
  , mMaster(nullptr)
  , mUnixAcceptor(mService)
  , mNextUnix(mService)
  , mPipeDevice(mService)
  , mFlags(0)
  , mStopped(false)
  , mBindIPs(nullptr)
  , mBindFeatureActive(false)
{
  // NOOP
}


ServicebrokerServer::~ServicebrokerServer()
{
  stop();
  mMaster = nullptr;
  mServerSockets.clear_and_dispose(TCPMixin::dispose);

  HTTPMixin::close();
}


void ServicebrokerServer::addClient(ServicebrokerClientBase& client)
{
  mClients.push_front(client);
}


void ServicebrokerServer::removeClient(ServicebrokerClientBase& client)
{
  mClients.remove_if(PointerComparePredicate(&client));
}


void ServicebrokerServer::removeClient(int32_t extendedID)
{
  mClients.remove_if_and_dispose(ExtendedIdPredicate(extendedID), ServicebrokerClientBase::dispose);
}


void ServicebrokerServer::stop()
{
  mStopped = true;
  mService.stop();
}


bool ServicebrokerServer::init(const char* mountpoint, MasterAdapter* master, unsigned int flags, const char* bindIPs)
{
  bool result = true;

  mMaster = master;
  mFlags = flags;
  mBindIPs = bindIPs;

  assert(strlen(mountpoint) + strlen(FND_SERVICEBROKER_ROOT) < UNIX_PATH_MAX);

  char abs_mountpoint[UNIX_PATH_MAX];
  strcpy(abs_mountpoint, FND_SERVICEBROKER_ROOT);
  strcat(abs_mountpoint, mountpoint);
  (void)::unlink(abs_mountpoint);

  // make sure the servicebroker is accessable by all, i.e. the socket is created using the
  // file access mode ugo+rwx
  mode_t old_mask = umask(0);

  result &= mUnixAcceptor.bind(Unix::Endpoint(abs_mountpoint)) == io::ok   
    && mUnixAcceptor.listen() == io::ok;

  // restore old umask setting
  (void)umask(old_mask);

  if (!result)
    Log::error("Cannot bind to mountpoint.");

  if (result && mMaster)
  {
    SignallingAddress addr;
    addr.ip.ip = getLocalIPForMasterConnection(mMaster->getMasterAddress());
    addr.ip.port = htons(getRealPort(SB_SLAVE_PORT));
    Servicebroker::getInstance().setSignallingAddress(addr);

    int internalNotifierFds[2];

    // we are slave
    if (::pipe(internalNotifierFds) == 0)
    {
      mWorker.start(internalNotifierFds[1], *mMaster);

      mPipeDevice = internalNotifierFds[0];
      mPipeDevice.async_read_all(&mCurrentNotification, sizeof(mCurrentNotification),
                        bind3(&ServicebrokerServer::handleInternalNotification, std::tr1::ref(*this),
                            std::tr1::placeholders::_1, std::tr1::placeholders::_2));
    }
    else
      result = false;
  }

  return result;
}


bool ServicebrokerServer::handleInternalNotification(size_t amount, io::error_code err)
{
  if (err == io::ok && amount == sizeof(InternalNotification))
    Servicebroker::getInstance().dispatch(mCurrentNotification.code, mCurrentNotification.value);

  return true;
}


TCPMixin* ServicebrokerServer::getServerSocket(IPv4::Endpoint& address)
{
  if (mBindFeatureActive)
  {
    for (intrusive::Slist<TCPMixin>::iterator it = mServerSockets.begin(); it != mServerSockets.end(); it++)
    {
      if (it.get()->mTcpAcceptor.getEndpoint().getIP() == address.getIP())
      {
        return it.get();
      }
    }
  }
  else
  {
    return &mServerSockets.front();
  }

  return nullptr;
}


bool ServicebrokerServer::handleNewTcpClient(IPv4::Endpoint& address, io::error_code err)
{
  TCPMixin* serverSocket = getServerSocket(address);

  if (!serverSocket)
  {
    char buf[32];
    Log::error("Not able to assign TCP client %s", address.toString(buf));
    return false;
  }
  if (err == io::ok)
  {
    (void)serverSocket->mNextTcp.setSocketOption(SocketTcpNoDelayOption<true>());

    char buf[32];
    Log::message(3, "Connected slave from %s", address.toString(buf));

    ServicebrokerClientTcp* newClient = new ServicebrokerClientTcp(
      serverSocket->mNextTcp, std::tr1::ref(*this), address.getIP(), serverSocket->mNextTcp.getEndpoint().getIP());
    newClient->start();
  }

  return true;
}


bool NotificationMixin::handleNewClient(IPv4::Endpoint& address, io::error_code err)
{
  if (err == io::ok)
  {
    (void)mNextNotification.setSocketOption(SocketTcpNoDelayOption<true>());

    // we are slave -> we handle 'pulses'
    char buf[32];
    Log::message(3, "Connected pulse socket from master %s", address.toString(buf));

    TCPMasterNotificationReceiver* newClient = new TCPMasterNotificationReceiver(mNextNotification);

    newClient->start();
  }

  return true;
}


bool NotificationMixin::init()
{
  if (!initSocket(mNotificationAcceptor, SB_SLAVE_PORT))
    return false;

  mNotificationAcceptor.async_accept(mNextNotification, bind3(
                            &NotificationMixin::handleNewClient,std::tr1::ref(*this),
                            std::tr1::placeholders::_1, std::tr1::placeholders::_2));
  return true;
}


bool ServicebrokerServer::handleNewLocalClient(Unix::Endpoint& /*address*/, io::error_code err)
{
  if (err == io::ok)
  {
    ServicebrokerClientUnix* newClient = new ServicebrokerClientUnix(mNextUnix, *this);
    newClient->start();
  }

  return true;
}


#ifdef HAVE_HTTP_SERVICE
bool HTTPMixin::handleNewClient(IPv4::Endpoint& /*address*/, io::error_code err)
{
  if (err == io::ok)
  {
    HTTPClient* newClient = new HTTPClient(mNextHTTP);
    newClient->start();
  }

  return true;
}


bool HTTPMixin::init()
{
  if (!initSocket(mHTTPAcceptor, SB_HTTP_PORT))
    return false;

  mHTTPAcceptor.async_accept(mNextHTTP, bind3(&HTTPMixin::handleNewClient, std::tr1::ref(*this),
                                std::tr1::placeholders::_1, std::tr1::placeholders::_2));
  return true;
}


void HTTPMixin::close()
{
  mHTTPAcceptor.close();
}


#endif   // HAVE_HTTP_SERVICE


int ServicebrokerServer::run()
{
  mUnixAcceptor.async_accept(mNextUnix, bind3(&ServicebrokerServer::handleNewLocalClient, std::tr1::ref(*this),
                                std::tr1::placeholders::_1, std::tr1::placeholders::_2));

  int rc = -1;

  int ips[MAX_IPS_TO_BIND];
  bool bindStatus[MAX_IPS_TO_BIND];
  unsigned short ipsToBind(0);
  unsigned short i(0);
  TCPMixin *serverSocket(nullptr);
  intrusive::Slist<TCPMixin>::iterator it;

  memset(bindStatus, false, sizeof(bindStatus));
  if (mBindIPs)
  {
    ipsToBind = parseIPs(mBindIPs, ips, sizeof(ips) / sizeof(ips[0]));
    mBindFeatureActive = (ipsToBind > 0);
  }
  else
  {
    ipsToBind = 1;//the server socket for all interfaces
  }
  for (i = 0; i < ipsToBind; i++)
  {
    serverSocket = new TCPMixin(mService);
    mServerSockets.push_front(*serverSocket);
  }

  for(; !mStopped; )
  {
    it = mServerSockets.begin();
    if (mFlags & eEnableTcpMaster)
    {
      if (!mBindFeatureActive)
      {
        if (!initSocket(it->mTcpAcceptor, SB_MASTER_PORT))
        {
          continue;
        }
        it->mTcpAcceptor.async_accept(it->mNextTcp, bind3(&ServicebrokerServer::handleNewTcpClient,
                                          std::tr1::ref(*this), std::tr1::placeholders::_1,
                                          std::tr1::placeholders::_2));
      }
      else
      {
        for(it = mServerSockets.begin(), i = 0; it != mServerSockets.end(); it++, i++ )
        {
          if (bindStatus[i])
          {
            continue;
          }
          if (!initSocket(it->mTcpAcceptor, SB_MASTER_PORT, ips[i]))
          {
            continue;
          }
          it->mTcpAcceptor.async_accept(it->mNextTcp, bind3(&ServicebrokerServer::handleNewTcpClient,
                                            std::tr1::ref(*this), std::tr1::placeholders::_1,
                                            std::tr1::placeholders::_2));
          bindStatus[i] = true;
          ipsToBind--;
        }
        if (ipsToBind)
        {
          continue;
        }
      }
    }

    if (mFlags & eEnableHTTP)
    {
      if (!HTTPMixin::init())
        continue;
    }

    // only accept connections when we are slave so we expect to get a connection from our master...
    if (mMaster)
    {
      if (mFlags & eEnableTcpSlave)
      {
        if (!NotificationMixin::init())
          continue;
      }
    }

    rc = mService.run();

    if (rc == -1)
      Log::error("POLL FAILED: errno=%d", errno);

    if (rc != 0)
    {
      Log::error("Will re-initialize acceptor sockets.");
    }
    else
      break;   // normal exit
  }

  return rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

