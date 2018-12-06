/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_SERVICEBROKER_FILELOCK_HPP
#define DSI_SERVICEBROKER_FILELOCK_HPP


/**
 * Advisory file lock RAII helper class.
 */
class FileLock
{
public:

   /**
    * Lock the given path.
    *
    * @param lockfile Filename to create a file and place an exclusive lock.
    */
   explicit
   FileLock(const char* lockfile);    

   /**
    * Removes the lock.
    */
   ~FileLock();   

   inline
   operator const void*() const
   {
      return mFd >= 0 ? this : nullptr;
   }

private:

   int mFd;
   char mLockfile[256];
};


#endif   // DSI_SERVICEBROKER_FILELOCK_HPP
