/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_PRIVATE_UTIL_HPP
#define DSI_PRIVATE_UTIL_HPP

#include <stdint.h>


namespace DSI
{
   /**
    * Creates a process unique id. This function is thread save.
    * @return a process wide unique id.
    */
   uint32_t createId();

}//namespace DSI

#endif //DSI_PRIVATE_UTIL_HPP
