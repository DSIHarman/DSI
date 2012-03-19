/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_PRIVATE_TDSI_TYPELIST_HPP
#define DSI_PRIVATE_TDSI_TYPELIST_HPP

namespace DSI
{

   namespace Private
   {

      /**
       * @struct SNilType TTypeList.hpp "dsi/TTypeList.hpp"
       *
       * An empty "end-of-list" type for meta programming purposes.
       *
       * @see TTypeList
       */
      struct SNilType {};


      /**
       * @struct TTypeList TTypeList.hpp "dsi/TTypeList.hpp"
       *
       * @brief Types container including manipulation functions
       *
       * In contrast to ordinary lists, a typelist does not store values but it stores types.
       * A typelist is a recursive template structure which ends in a SNilType.
       *
       * Example: to define a list containing an int, a double and a CHBString you may
       * write the following typedef:
       *
       * @code
       * typedef TTypeList<int, TTypeList<double, TTypeList<CHBString, SNilType> > > tMyListType;
       *
       * // this is equivalent to ...
       * typdef DSI_TYPELIST_3(int, double, CHBString) tMyListType;
       * @endcode
       *
       * @param HeadT The "list item" to insert.
       * @param TailT The "next pointer" to the next list item.
       *
       * @see @ref typelists_page
       */
      template<typename HeadT, typename TailT>
      struct TTypeList
      {
         typedef HeadT tHeadType;    ///< The "list item".
         typedef TailT tTailType;    ///< The "next pointer" to the next list item.
      };

   }   // namespace Private
}   // namespace DSI



/// define a typelist with one element
#define DSI_TYPELIST_1(t1) DSI::Private::TTypeList<t1, DSI::Private::SNilType>

/// define a typelist with two elements
#define DSI_TYPELIST_2(t1, t2) DSI::Private::TTypeList<t1, DSI_TYPELIST_1(t2) >

/// define a typelist with tree elements
#define DSI_TYPELIST_3(t1, t2, t3) DSI::Private::TTypeList<t1, DSI_TYPELIST_2(t2, t3) >

/// define a typelist with four elements
#define DSI_TYPELIST_4(t1, t2, t3, t4) DSI::Private::TTypeList<t1, DSI_TYPELIST_3(t2, t3, t4) >

/// define a typelist with five elements
#define DSI_TYPELIST_5(t1, t2, t3, t4, t5) DSI::Private::TTypeList<t1, DSI_TYPELIST_4(t2, t3, t4, t5) >

/// define a typelist with six elements
#define DSI_TYPELIST_6(t1, t2, t3, t4, t5, t6) DSI::Private::TTypeList<t1, DSI_TYPELIST_5(t2, t3, t4, t5, t6) >

/// define a typelist with seven elements
#define DSI_TYPELIST_7(t1, t2, t3, t4, t5, t6, t7) DSI::Private::TTypeList<t1, DSI_TYPELIST_6(t2, t3, t4, t5, t6, t7) >

/// define a typelist with eight elements
#define DSI_TYPELIST_8(t1, t2, t3, t4, t5, t6, t7, t8) DSI::Private::TTypeList<t1, DSI_TYPELIST_7(t2, t3, t4, t5, t6, t7, t8) >

/// define a typelist with nine elements
#define DSI_TYPELIST_9(t1, t2, t3, t4, t5, t6, t7, t8, t9) DSI::Private::TTypeList<t1, DSI_TYPELIST_8(t2, t3, t4, t5, t6, t7, t8, t9) >

/// define a typelist with ten elements
#define DSI_TYPELIST_10(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10) DSI::Private::TTypeList<t1, DSI_TYPELIST_9(t2, t3, t4, t5, t6, t7, t8, t9, t10) >

/// define a typelist with eleven elements
#define DSI_TYPELIST_11(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11) DSI::Private::TTypeList<t1, DSI_TYPELIST_10(t2, t3, t4, t5, t6, t7, t8, t9, t10, t11) >

/// define a typelist with twelve elements
#define DSI_TYPELIST_12(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12) DSI::Private::TTypeList<t1, DSI_TYPELIST_11(t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12) >

/// define a typelist with thirteen elements
#define DSI_TYPELIST_13(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13) DSI::Private::TTypeList<t1, DSI_TYPELIST_12(t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13) >

/// define a typelist with fourteen elements
#define DSI_TYPELIST_14(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14) DSI::Private::TTypeList<t1, DSI_TYPELIST_13(t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14) >

/// define a typelist with fifteen elements
#define DSI_TYPELIST_15(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15) DSI::Private::TTypeList<t1, DSI_TYPELIST_14(t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15) >

/// define a typelist with fifteen elements
#define DSI_TYPELIST_16(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16) TTypeList<t1, DSI_TYPELIST_15(t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16) >

// more to follow...


#endif   // DSI_PRIVATE_TDSI_TYPELIST_HPP
