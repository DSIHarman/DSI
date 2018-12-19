/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_PRIVATE_IF_HPP
#define DSI_PRIVATE_IF_HPP

#include <stdint.h>


namespace DSI
{
   template<bool condition, typename ThenT, typename ElseT>
   struct if_
   {
      typedef ElseT type;
   };

   template<typename ThenT, typename ElseT>
   struct if_<true, ThenT, ElseT>
   {
      typedef ThenT type;
   };

}//namespace DSI

#endif //DSI_PRIVATE_IF_HPP
