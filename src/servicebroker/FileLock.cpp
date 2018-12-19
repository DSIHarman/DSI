/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "FileLock.hpp"

#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <cstdio>
#include <cassert>

#include "Log.hpp"


FileLock::FileLock(const char* lockfile)
 : mFd(-1)    
{
  strncpy(mLockfile, lockfile, sizeof(mLockfile));
  mLockfile[sizeof(mLockfile)-1] = '\0';

  assert(lockfile && strlen(lockfile) > 0);

  mFd = ::open(lockfile, O_WRONLY|O_CREAT, 0666);
  if (mFd >= 0)
  {
    int ret = 0;
    while((ret = ::flock(mFd, LOCK_EX|LOCK_NB)) && errno == EINTR);

    if (ret < 0)
    {
      if (errno != EWOULDBLOCK)
      {
        int error = errno;
        Log::error("lockfile '%s': lock failed: %s", lockfile, ::strerror(error));
      }

      while(::close(mFd) && errno == EINTR);
      mFd = -1;
    }
    else
    {
      // success
      (void)::ftruncate(mFd, 0);

      char buf[10];
      ::sprintf(buf, "%d", ::getpid());

      // don't think about errors, this is just extra information not really needed
      (void)::write(mFd, buf, ::strlen(buf));
      (void)::fsync(mFd);
    }
  }
  else
  {
    int error = errno;
    Log::error("lockfile '%s': open/create failed: %s", lockfile, ::strerror(error));
  }
}


FileLock::~FileLock()
{
  if (mFd >= 0)
  {
    int ret = 0;
    while((ret = ::flock(mFd, LOCK_UN)) && errno == EINTR);

    if (ret < 0)
    {
      int error = errno;
      Log::error("lockfile '%s': flock(LOCK_UN) failed: %s", mLockfile, ::strerror(error));
    }

    while(::close(mFd) && errno == EINTR);

    (void)::unlink(mLockfile);
  }
}
