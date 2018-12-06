/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "clientIo.h"

#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include <malloc.h>

#include "frames.h"

#ifdef SB_NO_CRITICAL_SECTION
#   define sb_lock(lock) 
#   define sb_unlock(lock) 

#else
namespace /*anonymous*/
{
   pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;   
}

#   define sb_lock(lock) (void)pthread_mutex_lock(lock)
#   define sb_unlock(lock) (void)pthread_mutex_unlock(lock)

#endif   // SB_NO_CRITICAL_SECTION


extern "C" int sendAll(int fd, const void* data, size_t len)
{
   int have_error = 0;   
   size_t total = 0;  
   
   do
   {
      int rc = send(fd, ((const char*)data) + total, len - total, MSG_NOSIGNAL);
      if (rc < 0)
      {
         if (errno != EINTR)
         {
            have_error = 1;            
         }                     
      }
      else 
         total += rc;     
   }
   while(!have_error && total < len);
   
   return have_error;
}


extern "C" int recvAll(int fd, void* data, size_t len)
{   
   int have_error = 0;   
   size_t total = 0;  
   
   do
   {
      int rc = recv(fd, ((char*)data) + total, len - total, MSG_NOSIGNAL);
      if (rc < 0)
      {
         if (errno != EINTR)
         {
            have_error = 1;            
         }
      }
      else if (rc == 0)    // end of stream condition -> socket closed
      {
         // here we know we should get this amount of data!
         have_error = 1;         
      }        
      else 
         total += rc;                        
   }
   while(!have_error && total < len);
   
   return have_error;
}


extern "C" int sendiov(int fd, struct iovec* in, size_t len, size_t total_len)
{
   int have_error = 0;
   
   size_t total = 0; 
   
   struct msghdr hdr; 
   memset(&hdr, 0, sizeof(hdr));   
   
   hdr.msg_iov = in;
   hdr.msg_iovlen = len;
   
   do
   {         
      int rc = sendmsg(fd, &hdr, MSG_NOSIGNAL);      
      
      if (rc < 0)
      {
         if (errno != EINTR)
         {
            have_error = 1;            
         }         
      }
      else 
      {            
         total += rc;         
         
         if (total < total_len)
         {               
            // must adapt msghdr's iov array
            int empty_iovs = 0;
            int i;
            
            for(i = 0; i < (int)hdr.msg_iovlen && rc > 0; ++i)
            {               
               if ((unsigned int)rc < in[i].iov_len)
               {
                  in[i].iov_base = ((char*)in[i].iov_base) + rc;
                  in[i].iov_len -= rc;
                  break;
               }
               else
               {
                  rc -= in[i].iov_len;               
                  ++empty_iovs;                  
               }               
            }
                        
            // remove already sent iov's
            hdr.msg_iov += empty_iovs;
            hdr.msg_iovlen -= empty_iovs;
         }
      }      
   }
   while(!have_error && total < total_len);    

   return have_error;
}


extern "C" int sendAndReceiveV(int fd, dcmd_t cmd, size_t inLen, size_t outLen, struct iovec* in, struct iovec* out, int* rc)
{ 
   int retval = -1;
   errno = EINVAL;   
      
   int have_error = 0;   // assume no error
   
   // send all input data in one big request frame
   {      
      size_t total_body_length = 0;
      size_t i;      
    
      // calculate complete request length    
      for(i = 0; i<inLen; ++i)
      {
         total_body_length += in[i].iov_len;
      }
            
      sb_request_frame_t f;
      INIT_REQUEST_FRAME(&f, total_body_length, cmd);      
      
      // lock the socket until we have finished operation
      sb_lock(&lock);
         
      // try to send via sendmsg on Linux and QNX, if an error occurs
      // get back to sending each frame individually      
      struct iovec* iov = nullptr;
      
      if (in[0].iov_base == nullptr)
      {
         // later, fill placeholder with request frame structure                  
         iov = in;         
      }
      else
      {
         // must make a copy of the iov structure
         iov = (struct iovec*)malloc(sizeof(struct iovec) * (inLen + 1));         
         if (iov)
         {
            memcpy(iov+1, in, inLen * sizeof(struct iovec));
            ++inLen;
         }
         else
            have_error = 1;
      } 

      if (have_error == 0)
      {         
         // add header to iov structure
         iov[0].iov_base = &f;
         iov[0].iov_len = sizeof(f);      
                  
         have_error = sendiov(fd, iov, inLen, total_body_length + sizeof(f));      
               
         if (iov != in)
            free(iov);
      }           
   }
   
   // receive all frames from response into out-iov
   if (!have_error)
   {
      sb_response_frame_t f;
      
      // receive frame envelope
      have_error = recvAll(fd, &f, sizeof(f));
      
      // check magic      
      if (f.fr_envelope.fr_magic != SB_FRAME_MAGIC)
         have_error = 1;      
      
      // set return code if successfully read
      if (!have_error && rc)
      {         
         *rc = f.fr_returncode;
      }
      
      // read data back to sender
      size_t rest = f.fr_envelope.fr_size;
      
      // now read complete response body into the iovs' structures, but not more than the received length from request
      size_t i;
      for (i = 0; i<outLen && !have_error && rest > 0; ++i)
      {                
         size_t cur = rest > out[i].iov_len ? out[i].iov_len : rest;
         have_error = recvAll(fd, out[i].iov_base, cur);         
         
         rest -= cur;
      }      
   }     
   
   if (!have_error)
      retval = 0;
      
   // lock the socket until we have finished operation
   sb_unlock(&lock);       

   return retval;
}


extern "C" int sendAndReceive(int fd, int cmd, void* ptr, size_t inputlen, size_t outputlen, int* rc)
{
   // let the first iov be zero, this is a placeholder for the frame so we
   // don't need to copy the whole iov structure yet another time
   struct iovec in[2];
   memset(in, 0, sizeof(in));
   
   in[1].iov_base = ptr;
   in[1].iov_len = inputlen;
   
   if (outputlen > 0)
   {
      struct iovec out;
      out.iov_base = ptr;
      out.iov_len = outputlen;      
      
      return sendAndReceiveV(fd, cmd, 2, 1, in, &out, rc);
   }
   else   
      return sendAndReceiveV(fd, cmd, 2, 0, in, nullptr, rc);
}


extern "C" int sendAuthPackage(int fd, int sendCredentials)
{
   // ok, pass credentials to the servicebroker
   int ret = -1;  
            
   struct auth
   {
      char magic[4];
      int32_t pid;
   } auth_request;
  
   memcpy(auth_request.magic, "AUTH", 4);
   auth_request.pid = getpid();
 
   struct iovec io[1];
   io[0].iov_base = &auth_request;
   io[0].iov_len = sizeof(auth_request);

   struct msghdr hdr;
   memset(&hdr, 0, sizeof(hdr));
   hdr.msg_iov = io;
   hdr.msg_iovlen = 1;
   
   if (sendCredentials)
   {
      char buf[CMSG_SPACE(sizeof(struct ucred))];
     
      hdr.msg_control = buf;
      hdr.msg_controllen = sizeof(buf);   
      
      struct cmsghdr* cmsg = CMSG_FIRSTHDR(&hdr);
      cmsg->cmsg_level = SOL_SOCKET;
      cmsg->cmsg_type = SCM_CREDENTIALS;
      cmsg->cmsg_len = CMSG_LEN(sizeof(struct ucred));
         
      struct ucred* credentials = (struct ucred*)CMSG_DATA(cmsg);
      credentials->pid = getpid();
      credentials->uid = geteuid();
      credentials->gid = getegid();
   }
         
   do 
   {
      ret = sendmsg(fd, &hdr, MSG_NOSIGNAL);
   } 
   while (ret < 0 && errno == EINTR);

   if (ret == sizeof(auth_request))
      ret = 0;

   return ret;
}


extern "C" const char* make_unix_path(char* buf, size_t len, int pid, int64_t chid)
{
   if (buf && len > 20)
   {
      memset(buf, 0, len);
      sprintf(buf+1, "%d/%ld", pid, chid);
      return buf;
   }
   
   return nullptr;
}
