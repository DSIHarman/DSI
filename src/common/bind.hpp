/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_COMMON_BIND_HPP
#define DSI_COMMON_BIND_HPP


#include <tr1/functional>


namespace DSI
{
   /// silly wrapper class around tr1 binders since these do not allow to be called with rvalues...
   template<typename BinderT>
   struct Wrapper
   {
      typedef typename BinderT::result_type result_type;

      inline
      Wrapper(const BinderT& b)
         : b_(b)
      {
         // NOOP
      }

      template<typename T>
      inline
      result_type operator()(T t)
      {
         return b_(t);
      }

      template<typename T1, typename T2>
      inline
      result_type operator()(T1 t1, T2 t2)
      {
         return b_(t1, t2);
      }

      template<typename T1, typename T2, typename T3>
      inline
      result_type operator()(T1 t1, T2 t2, T3 t3)
      {
         return b_(t1, t2, t3);
      }

   private:

      BinderT b_;
   };


   template<typename T>
   inline
   Wrapper<T> wrap(const T& t)
   {
      return Wrapper<T>(t);
   }

}//namespace DSI


/// Wrap tr1 binder with one argument excluding the bound function.
/// This could be either a this pointer of a member function taking zero arguments or an ordinary function with one argument.
#define bind1(f, a1)         DSI::wrap(std::tr1::bind(f, a1))

/// Wrap tr1 binder with two arguments excluding the bound function.
/// This could be either a this pointer of a member function taking one argument or an ordinary function with two arguments.
#define bind2(f, a1, a2)     DSI::wrap(std::tr1::bind(f, a1, a2))

/// Wrap tr1 binder with three arguments excluding the bound function.
/// This could be either a this pointer of a member function taking two arguments or an ordinary function with three arguments.
#define bind3(f, a1, a2, a3) DSI::wrap(std::tr1::bind(f, a1, a2, a3))


#endif   // DSI_COMMON_BIND_HPP
