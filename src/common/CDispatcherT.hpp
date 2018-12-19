/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_DISPATCHERT_HPP
#define DSI_DISPATCHERT_HPP


namespace DSI
{
   template<typename InheriterT>
   int DispatcherBase<InheriterT>::run()
   {
      int ret = 0;

      // reset exit condition
      finished_ = false;
      rc_ = 0;

      while(!finished_ && ret >= 0)
      {
         ret = static_cast<InheriterT*>(this)->poll(1000);
      }

      if (!finished_ && ret < 0)
         rc_ = -1;

      return rc_;
   }
}//namespace DSI

#endif   // DSI_DISPATCHERT_HPP
