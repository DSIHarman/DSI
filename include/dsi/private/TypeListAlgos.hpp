/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_PRIVATE_TYPELISTALGOS_HPP
#define DSI_PRIVATE_TYPELISTALGOS_HPP


#include "dsi/private/TTypeList.hpp"

namespace DSI
{

   namespace Private
   {

      // ---------------------------------- length of TTypeList ------------------------------------------


      /**
       * Typelist algorithms for metaprogramming issues on TTypeList.
       */
      namespace TypeListAlgos
      {

         /**
          * @struct Size TTypeList.hpp "dsi/TTypeList.hpp"
          *
          * Calculate the size of a given TTypeList. The result is returned as the unnamed enumerator 'value'.
          *
          * @param ListT The typelist, for which the length should be calcalated.
          */
         template<typename ListT>
         struct Size;


         /// @internal
         template<typename HeadT, typename TailT>
         struct Size<TTypeList<HeadT, TailT> >
         {
            enum { value = Size<TailT>::value + 1 };
         };

         /// @internal
         template<typename HeadT>
         struct Size<TTypeList<HeadT, SNilType> >
         {
            enum { value = 1 };
         };

         /// @internal
         template<>
         struct Size<TTypeList<SNilType, SNilType> >
         {
            enum { value = 0 };
         };


         // -------------------------------- find type within typelist -------------------------------------------


         /**
          * @struct Find TTypeList.hpp "dsi/TTypeList.hpp"
          *
          * Find position of given type in typelist. The result is returned as anonymous enumerator 'value'.
          * If the desired type cannot be found, -1 is returned.
          */
         template<typename SearchT, typename ListT, int N=0>
         struct Find;


         /// @internal
         template<typename SearchT, typename HeadT, typename TailT, int N>
         struct Find<SearchT, TTypeList<HeadT, TailT>, N>
         {
            enum { value = Find<SearchT, TailT, N+1>::value };
         };

         /// @internal
         template<typename SearchT, typename TailT, int N>
         struct Find<SearchT, TTypeList<SearchT, TailT>, N>
         {
            enum { value = N };
         };

         /// @internal
         template<typename SearchT, typename HeadT, int N>
         struct Find<SearchT, TTypeList<HeadT, SNilType>, N>
         {
            enum { value = -1 };
         };

         template<typename SearchT, int N>
         struct Find<SearchT, TTypeList<SearchT, SNilType>, N>
         {
            enum { value = N };
         };


         // -------------------------------- type at given position -------------------------------------------

         /**
          * @struct TypeAt TTypeList.hpp "dsi/TTypeList.hpp"
          *
          * Extract type at given position in TTypeList; position must be within bounds. The result is
          * returned as typedef 'type' and starts with position 0.
          */
         template<int N, typename ListT>
         struct TypeAt;

         /// @internal
         template<int N, typename HeadT, typename TailT>
         struct TypeAt<N, TTypeList<HeadT, TailT> >
         {
            typedef typename TypeAt<N-1, TailT>::type type;
         };

         /// @internal
         template<typename HeadT, typename TailT>
         struct TypeAt<0, TTypeList<HeadT, TailT> >
         {
            typedef HeadT type;
         };


         // ----------------- type at (relaxed, does not matter if boundary was crossed) ---------------------

         /**
          * @struct RelaxedTypeAt TTypeList.hpp "dsi/TTypeList.hpp"
          *
          * Extract type at given position in TTypeList; if position is out of
          * TTypeList bounds it always returns the type SNilType. The result is
          * returned as typedef 'type' and starts with position 0.
          */
         template<int N, typename ListT>
         struct RelaxedTypeAt;


         /// @internal
         template<int N>
         struct RelaxedTypeAt<N, SNilType>
         {
            typedef SNilType type;   ///< save access over end of TTypeList
         };

         /// @internal
         template<int N, typename HeadT, typename TailT>
         struct RelaxedTypeAt<N, TTypeList<HeadT, TailT> >
         {
            typedef typename RelaxedTypeAt<N-1, TailT>::type type;
         };

         /// @internal
         template<typename HeadT, typename TailT>
         struct RelaxedTypeAt<0, TTypeList<HeadT, TailT> >
         {
            typedef HeadT type;
         };


         // ------------------------------ push/pop front -------------------------------------

         /**
          * @struct PushFront TTypeList.hpp "dsi/TTypeList.hpp"
          *
          * Add a type at the beginning of the list. The new list is
          * returned as typedef 'type'.
          */
         template<typename ListT, typename InsertT>
         struct PushFront
         {
            typedef TTypeList<InsertT, ListT> type;
         };


         /**
          * @struct PopFront TTypeList.hpp "dsi/TTypeList.hpp"
          *
          * Removes the first element of the typelist and return the new list via 'type'.
          */
         template<typename ListT>
         struct PopFront;

         /// @internal
         template<typename HeadT, typename TailT>
         struct PopFront<TTypeList<HeadT, TailT> >
         {
            typedef TailT type;
         };

         /// @internal
         template<>
         struct PopFront<SNilType>
         {
            typedef SNilType type;
         };


         // -------------------------------push/pop back ------------------------------------

         /**
          * @struct PushBack TTypeList.hpp "dsi/TTypeList.hpp"
          *
          * Append a type at the end of the list. The new list is
          * returned as typedef 'type'.
          *
          * @param ListT   The list that should be appended.
          * @param InsertT The type to be appended.
          * @return the resulting appended list via @c type.
          */
         template<typename ListT, typename InsertT>
         struct PushBack;

         /// @internal
         template<typename HeadT, typename TailT, typename InsertT>
         struct PushBack<TTypeList<HeadT, TailT>, InsertT>
         {
            typedef TTypeList<HeadT, typename PushBack<TailT, InsertT>::type> type;
         };

         /// @internal
         template<typename HeadT, typename InsertT>
         struct PushBack<TTypeList<HeadT, SNilType>, InsertT>
         {
            typedef TTypeList<HeadT, TTypeList<InsertT, SNilType> > type;
         };


         /**
          * @struct PopBack TTypeList.hpp "dsi/TTypeList.hpp"
          *
          * Removes the last element of the typelist and return the new list via 'type'.
          */
         template<typename ListT>
         struct PopBack;

         /// @internal
         template<typename HeadT, typename TailT>
         struct PopBack<TTypeList<HeadT, TailT> >
         {
            typedef TTypeList<HeadT, typename PopBack<TailT>::type> type;
         };

         /// @internal
         template<typename HeadT>
         struct PopBack<TTypeList<HeadT, SNilType> >
         {
            typedef SNilType type;
         };

         /// @internal
         template<>
         struct PopBack<SNilType>
         {
            // FIXME maybe generate a compiler error here?!
            typedef SNilType type;
         };


         // ----------------------------- generic algorithms ----------------------------------


         /**
          * @struct SHBMax TTypeList.hpp "dsi/TTypeList.hpp"
          *
          * The SHBMax algorithm can be used to iterate over a typelist and call a functional on each
          * contained element which must consist of a metafunction @c apply_ and return a @c value.
          * The maximum of all calculated values will be returned as @c value. E.g.
          *
          * @code
          *   typedef TYPELIST_2(int, double) tListType;
          *
          *   // return the maximum size of all contained elements...
          *   CPPUNIT_ASSERT_EQUAL(8, SHBMax<tListType, SHBSizeFunc>::value);
          * @endcode
          *
          * @todo the algorithm should look a little bit like max_element and return an index of the maximum element as given by the Functor
          */
         template<typename ListT, typename FuncT>
         struct SHBMax;

         ///@internal
         template<typename HeadT, typename TailT, typename FuncT>
         struct SHBMax<TTypeList<HeadT, TailT>, FuncT>
         {
            typedef typename FuncT::template apply_<HeadT> first_type__;
            enum { value1__ = first_type__::value };
            enum { value2__ = SHBMax<TailT, FuncT>::value };

            enum { value = (int)value1__ > (int)value2__ ? (int)value1__ : (int)value2__ };
         };


         ///@internal
         template<typename HeadT, typename FuncT>
         struct SHBMax<TTypeList<HeadT, SNilType>, FuncT>
         {
            typedef typename FuncT::template apply_<HeadT> type__;
            enum { value = type__::value };
         };


      }   // namespace TypeListAlgos

   }   // namespace Private

} // namespace DSI
#endif   //DSI_PRIVATE_TYPELISTALGOS_HPP
