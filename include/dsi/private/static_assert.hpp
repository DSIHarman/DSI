/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_PRIVATE_STATIC_ASSERT_HPP
#define DSI_PRIVATE_STATIC_ASSERT_HPP


/**
 * @internal 
 */
#define __DSI_STATIC_ASSERT_CONCAT2(a,b) a ## b

/**
 * @internal 
 */
#define __DSI_STATIC_ASSERT_CONCAT(a,b) __DSI_STATIC_ASSERT_CONCAT2(a,b)


/**
 * @internal 
 *
 * This code could be removed in favour of tr1 c++11 implementation.
 */
#define DSI_STATIC_ASSERT(condition) \
   char __DSI_STATIC_ASSERT_CONCAT(checkArray, __LINE__)[condition ? 1 : -1]; \
   (void)__DSI_STATIC_ASSERT_CONCAT(checkArray, __LINE__);

   
#endif   // DSI_PRIVATE_STATIC_ASSERT_HPP