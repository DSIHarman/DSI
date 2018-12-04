/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_PRIVATE_TVARIANT_HPP
#define DSI_PRIVATE_TVARIANT_HPP

#ifndef DSI_TVARIANT_HPP
#   error "This is an internal header, do not include directly!"
#endif

#include "dsi/private/TypeListAlgos.hpp"

namespace DSI
{
   
   /// @internal
   namespace Private
   {

      /**  
       * A alignment calculating metafunction. @c SHBAlignFunc::apply_<T>::value will be the
       * alignment of @c T.
       */
      struct SHBAlignFunc
      {
         template<typename T>
         struct apply_
         {
            enum { value = std::tr1::alignment_of<T>::value };
         };
      };


      /**  
       * A sizeof() calculating metafunction. @c SHBSizeFunc::apply_<T>::value will be the
       * size of @c T.
       */
      struct SHBSizeFunc
      {
         template<typename T>
         struct apply_
         {
            enum { value = sizeof(T) };
         };
      };


      // template forward decl
      template<typename T1, typename T2, typename T3, typename T4, typename T5, 
               typename T6, typename T7, typename T8, typename T9, typename T10, 
               typename T11, typename T12, typename T13, typename T14, typename T15>
      struct TVariantHelper;


      // construction function 
      template<typename T>
      void variant_destroy(void* t)
      { 
         ((T*)t)->~T();
      }


      // destruction function 
      template<typename T>
      void variant_construct(void* to, const void* from)
      {   
         ::new(to) T(*(const T*)from);
      }


      ///@internal
      template<typename ParamT>
      class Callfunc
      {
      public:

         template<typename VisitorT, typename VariantT>
         static
         typename VisitorT::return_type eval(VisitorT& visitor, VariantT& variant)
         {
            return visitor(*variant.template get<ParamT>());
         }
      };


      ///@internal empty variant call function object
      template<>
      class Callfunc<void>
      {
      public:

         template<typename VisitorT, typename VariantT>
         static
         typename VisitorT::return_type eval(VisitorT& visitor, VariantT&)
         {
            return visitor();
         }
      };


      ///@internal
      template<>
      class Callfunc<SNilType>
      {
      public:

         template<typename VisitorT, typename VariantT>
         static
         typename VisitorT::return_type eval(VisitorT& visitor, VariantT&)
         {
            assert(false);
            return visitor();
         }
      };


      // implementation for all 15 elements possible
      template<typename T1, typename T2, typename T3, typename T4, typename T5, 
               typename T6, typename T7, typename T8, typename T9, typename T10, 
               typename T11, typename T12, typename T13, typename T14, typename T15>
      class TVariantHelper
      {
      public:

         static inline 
         void construct(signed char i, void* lhs, const void* rhs)
         {            
            typedef void(*cfunc_type)(void*, const void*);      
            
            static const cfunc_type funcs[] = {
               &variant_construct<T1>,
               &variant_construct<T2>,
               &variant_construct<T3>,
               &variant_construct<T4>,
               &variant_construct<T5>,
               &variant_construct<T6>, 
               &variant_construct<T7>,
               &variant_construct<T8>,
               &variant_construct<T9>,
               &variant_construct<T10>,
               &variant_construct<T11>,
               &variant_construct<T12>,
               &variant_construct<T13>,
               &variant_construct<T14>,
               &variant_construct<T15>       
               // append if necessary         
            };
      
            funcs[i - 1](lhs, rhs);      
         }
   
         static inline 
         void destruct(signed char i, void* obj)
         {      
            // Beware that the direction of types in the typelist is in reverse order!!!
            typedef void(*func_type)(void*);
            
            static const func_type funcs[] = {
               &variant_destroy<T1>,
               &variant_destroy<T2>,
               &variant_destroy<T3>,   
               &variant_destroy<T4>,
               &variant_destroy<T5>,
               &variant_destroy<T6>,
               &variant_destroy<T7>,
               &variant_destroy<T8>,
               &variant_destroy<T9>,
               &variant_destroy<T10>,
               &variant_destroy<T11>,
               &variant_destroy<T12>,
               &variant_destroy<T13>,
               &variant_destroy<T14>,
               &variant_destroy<T15>         
               // append if necessary
            };
      
            funcs[i - 1](obj);
         }   
   
         template<typename VisitorT, typename VariantT>
         static inline
         typename VisitorT::return_type eval(signed char i, VisitorT& visitor, VariantT& variant)
         {
            typedef typename VisitorT::return_type (*func_type)(VisitorT&, VariantT&);   
      
            static const func_type funcs[] = {
               &Callfunc<void>::eval,      // empty variant
               &Callfunc<T1>::eval,
               &Callfunc<T2>::eval,
               &Callfunc<T3>::eval,
               &Callfunc<T4>::eval,  
               &Callfunc<T5>::eval,
               &Callfunc<T6>::eval,
               &Callfunc<T7>::eval,
               &Callfunc<T8>::eval,
               &Callfunc<T9>::eval,
               &Callfunc<T10>::eval,
               &Callfunc<T11>::eval,
               &Callfunc<T12>::eval,
               &Callfunc<T13>::eval,
               &Callfunc<T14>::eval,
               &Callfunc<T15>::eval         
            };   
            
            return funcs[i](visitor, variant);
         }
      };


      // implementation for two elements
      template<typename T1, typename T2>
      class TVariantHelper<T1, T2, SNilType, SNilType, SNilType, 
                           SNilType, SNilType, SNilType, SNilType, SNilType, 
                           SNilType, SNilType, SNilType, SNilType, SNilType>
      {
      public:

         static inline 
         void construct(signed char i, void* lhs, const void* rhs)
         {            
            typedef void(*cfunc_type)(void*, const void*);      
      
            static const cfunc_type funcs[] = {
               &variant_construct<T1>,
               &variant_construct<T2>         
            };            
      
            funcs[i - 1](lhs, rhs);      
         }
   
         static inline 
         void destruct(signed char i, void* obj)
         {      
            // Beware that the direction of types in the typelist is in reverse order!!!
            typedef void(*func_type)(void*);
            
            static const func_type funcs[] = {
               &variant_destroy<T1>,
               &variant_destroy<T2>         
            };      
      
            funcs[i - 1](obj);
         }   
   
         template<typename VisitorT, typename VariantT>
         static inline
         typename VisitorT::return_type eval(signed char i, VisitorT& visitor, VariantT& variant)
         {
            typedef typename VisitorT::return_type (*func_type)(VisitorT&, VariantT&);   
      
            static const func_type funcs[] = {
               &Callfunc<void>::eval,      // empty variant
               &Callfunc<T1>::eval,
               &Callfunc<T2>::eval         
            };   
            
            return funcs[i](visitor, variant);
         }
      };


      // implementation for three elements
      template<typename T1, typename T2, typename T3>
      class TVariantHelper<T1, T2, T3, SNilType, SNilType, 
                           SNilType, SNilType, SNilType, SNilType, SNilType, 
                           SNilType, SNilType, SNilType, SNilType, SNilType>
      {
      public:

         static inline 
         void construct(signed char i, void* lhs, const void* rhs)
         {            
            typedef void(*cfunc_type)(void*, const void*);      
      
            static const cfunc_type funcs[] = {
               &variant_construct<T1>,
               &variant_construct<T2>,         
               &variant_construct<T3>
            };      
      
            funcs[i - 1](lhs, rhs);      
         }
   
         static inline 
         void destruct(signed char i, void* obj)
         {      
            // Beware that the direction of types in the typelist is in reverse order!!!
            typedef void(*func_type)(void*);
            
            static const func_type funcs[] = {
               &variant_destroy<T1>,
               &variant_destroy<T2>,         
               &variant_destroy<T3>
            };
      
            funcs[i - 1](obj);
         }   
   
         template<typename VisitorT, typename VariantT>
         static inline
         typename VisitorT::return_type eval(signed char i, VisitorT& visitor, VariantT& variant)
         {
            typedef typename VisitorT::return_type (*func_type)(VisitorT&, VariantT&);   
      
            static const func_type funcs[] = {
               &Callfunc<void>::eval,      // empty variant
               &Callfunc<T1>::eval,
               &Callfunc<T2>::eval,
               &Callfunc<T3>::eval
            };   
            
            return funcs[i](visitor, variant);
         }
      };


      // implementation for four elements
      template<typename T1, typename T2, typename T3, typename T4>
      class TVariantHelper<T1, T2, T3, T4, SNilType, 
                           SNilType, SNilType, SNilType, SNilType, SNilType, 
                           SNilType, SNilType, SNilType, SNilType, SNilType>
      {
      public:

         static inline 
         void construct(signed char i, void* lhs, const void* rhs)
         {            
            typedef void(*cfunc_type)(void*, const void*);      
      
            static const cfunc_type funcs[] = {
               &variant_construct<T1>,
               &variant_construct<T2>,         
               &variant_construct<T3>,
               &variant_construct<T4>
            };
      
            funcs[i - 1](lhs, rhs);      
         }
   
         static inline 
         void destruct(signed char i, void* obj)
         {      
            // Beware that the direction of types in the typelist is in reverse order!!!
            typedef void(*func_type)(void*);
            
            static const func_type funcs[] = {
               &variant_destroy<T1>,
               &variant_destroy<T2>,         
               &variant_destroy<T3>,
               &variant_destroy<T4>
            };
      
            funcs[i - 1](obj);
         }   
   
         template<typename VisitorT, typename VariantT>
         static inline
         typename VisitorT::return_type eval(signed char i, VisitorT& visitor, VariantT& variant)
         {
            typedef typename VisitorT::return_type (*func_type)(VisitorT&, VariantT&);   
      
            static const func_type funcs[] = {
               &Callfunc<void>::eval,      // empty variant
               &Callfunc<T1>::eval,
               &Callfunc<T2>::eval,
               &Callfunc<T3>::eval,
               &Callfunc<T4>::eval         
            };   
            
            return funcs[i](visitor, variant);
         }
      };


      // implementation for five elements
      template<typename T1, typename T2, typename T3, typename T4, typename T5>
      class TVariantHelper<T1, T2, T3, T4, T5, 
                           SNilType, SNilType, SNilType, SNilType, SNilType, 
                           SNilType, SNilType, SNilType, SNilType, SNilType>
      {
      public:

         static inline 
         void construct(signed char i, void* lhs, const void* rhs)
         {            
            typedef void(*cfunc_type)(void*, const void*);      
      
            static const cfunc_type funcs[] = {
               &variant_construct<T1>,
               &variant_construct<T2>,         
               &variant_construct<T3>,
               &variant_construct<T4>,
               &variant_construct<T5>
            };
      
            funcs[i - 1](lhs, rhs);      
         }
   
         static inline 
         void destruct(signed char i, void* obj)
         {      
            // Beware that the direction of types in the typelist is in reverse order!!!
            typedef void(*func_type)(void*);
            
            static const func_type funcs[] = {
               &variant_destroy<T1>,
               &variant_destroy<T2>,         
               &variant_destroy<T3>,
               &variant_destroy<T4>,
               &variant_destroy<T5>
            };
      
            funcs[i - 1](obj);
         }   
   
         template<typename VisitorT, typename VariantT>
         static inline
         typename VisitorT::return_type eval(signed char i, VisitorT& visitor, VariantT& variant)
         {
            typedef typename VisitorT::return_type (*func_type)(VisitorT&, VariantT&);   
      
            static const func_type funcs[] = {
               &Callfunc<void>::eval,      // empty variant
               &Callfunc<T1>::eval,
               &Callfunc<T2>::eval,
               &Callfunc<T3>::eval,
               &Callfunc<T4>::eval,  
               &Callfunc<T5>::eval         
            };   
            
            return funcs[i](visitor, variant);
         }
      };


   }   // namespace Private
}   // namespace DSI


#endif   // DSI_PRIVATE_TVARIANT_HPP
