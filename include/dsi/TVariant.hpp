/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_TVARIANT_HPP
#define DSI_TVARIANT_HPP


#include <tr1/type_traits>
#include <cassert>


#include "dsi/private/TVariant.hpp"

namespace DSI
{

/**
 * @class    TVariant TVariant.hpp "dsi/TVariant.hpp"
 * @brief    C union like container for C++ objects
 *
 * The TVariant is a kind of union for C++ objects. It can contain an object of at least two different types.
 * Example:
 *
 * @code
 * // create an emty variant
 * TVariant<TYPELIST_2(int, double)> v;
 *
 * // variant now stores a double value
 * v = 3.1415;
 *
 * // you may access the stored data with the accessor function.
 * printf("%f\n", *(v.get<double>()));
 *
 * // accessing the data as int is an error here
 * // v.get<int>() is 0 now
 *
 * // reset the variant to store an int value instead of the double. The double will be destructed in appropriate C++ manner.
 * v = 42;
 * @endcode
 *
 * The @c TStaticVisitor helps using a TVariant in a object oriented context.
 */
   template<typename TypelistT>
   class TVariant
   {
      template<typename VisitorT, typename VariantT>
      friend typename VisitorT::return_type staticVisit(VisitorT&, VariantT&);
      
   public:

      // FIXME make sure not to be able to add the same type multiple times
      typedef TypelistT tTypelistType;

      enum { unset = 0 };

      /**
       * Retrieve the typeId for the given template argument. This can be used in conjunction with the
       * @c getTypeId function in order to construct a switch-case statement. Example:
       *
       * @code
       * typedef TVariant<int, double> tMyVariantType;
       * tMyVariantType v;
       *
       * switch(v.getTypeId())
       * {
       * case tMyVariantType::typeIdOf<int>::value:
       *    ...
       * case tMyVariantType::typeIdOf<double>::value:
       *    ...
       * case tMyVariantType::unset:
       *    ...
       * default:
       *    // can never happen
       * }
       * @endcode
       *
       * @note Using a visitor should be preferred to a switch-case clause.
       * @see staticVisit
       */
      template<typename T>
      struct typeIdOf
      {
         enum { value = Private::TypeListAlgos::Find<T, tTypelistType>::value + 1 };
      };

   private:      
   
      typedef Private::TVariantHelper<
         typename Private::TypeListAlgos::RelaxedTypeAt<0, tTypelistType>::type,
         typename Private::TypeListAlgos::RelaxedTypeAt<1, tTypelistType>::type,
         typename Private::TypeListAlgos::RelaxedTypeAt<2, tTypelistType>::type,
         typename Private::TypeListAlgos::RelaxedTypeAt<3, tTypelistType>::type,
         typename Private::TypeListAlgos::RelaxedTypeAt<4, tTypelistType>::type,
         typename Private::TypeListAlgos::RelaxedTypeAt<5, tTypelistType>::type,
         typename Private::TypeListAlgos::RelaxedTypeAt<6, tTypelistType>::type,
         typename Private::TypeListAlgos::RelaxedTypeAt<7, tTypelistType>::type,
         typename Private::TypeListAlgos::RelaxedTypeAt<8, tTypelistType>::type,
         typename Private::TypeListAlgos::RelaxedTypeAt<9, tTypelistType>::type,
         typename Private::TypeListAlgos::RelaxedTypeAt<10, tTypelistType>::type,
         typename Private::TypeListAlgos::RelaxedTypeAt<11, tTypelistType>::type,
         typename Private::TypeListAlgos::RelaxedTypeAt<12, tTypelistType>::type,
         typename Private::TypeListAlgos::RelaxedTypeAt<13, tTypelistType>::type,
         typename Private::TypeListAlgos::RelaxedTypeAt<14, tTypelistType>::type
      > tHelperType;

   public:
      
      enum { size = Private::TypeListAlgos::SHBMax<tTypelistType, Private::SHBSizeFunc>::value };         ///< Size requirements of the variant data.
      enum { alignment = Private::TypeListAlgos::SHBMax<tTypelistType, Private::SHBAlignFunc>::value };   ///< Alignment requirements of the variant data.

      /**
       * Construct an uninitialized variant.
       */
      inline
      TVariant()
         : mIdx(unset)
      {
         // NOOP
      }

      /**
       * Destructs the internal data (if any).
       */
      inline
      ~TVariant()
      {
         try_destroy();
      }

      /**
       * Construct a variant from given data t.
       *
       * @param t The argument that should be stored inside the variant.
       */
      template<typename T>
      explicit
      TVariant(const T& t);

      /**
       * Assignment operator.
       *
       * @param t The argument that should be stored inside the variant.
       * @return a reference to this.
       */
      template<typename T>
      TVariant& operator=(const T& t);

      /**
       * Equality operator.
       *
       * @param rhs The rhs of the equality operator.
       * @return true if rhs and *this are equal, else false.
       */
      bool operator==(const TVariant& rhs) const;

      /**
       * Inequality operator.
       *
       * @param rhs The rhs of the inequality operator.
       * @return false if rhs and *this are equal, else true.
       */
      inline
      bool operator!=(const TVariant& rhs) const
      {
         return !(*this == rhs);
      }

      /**
       * Copy constructor.
       *
       * @param v The rhs of the copy construtor.
       */
      inline
      TVariant(const TVariant& v)
      {
         construct_from(v);
      }

      /**
       * Assignment operator.
       *
       * @param v The rhs of the copy construtor.
       * @return a reference to this.
       */
      TVariant& operator=(const TVariant& v);

      /**
       * Destruct the stored object (if any).
       */
      void reset();

      /**
       * Destruct the stored object (if any) and construct a new one from the given inplacer argument.
       * Include the @c THBInplaceFactory.hpp header file in order to use the inplacing functionality:
       *
       * @code
       * TVariant<int, CHBString> v;
       *
       * v = 42;                                                 // copy construct from int
       * ...
       * v.reset<CHBString>(inplace("Hallo Welt"));              // inplace construct: call constructor of CHBString
       *                                                         // with const char* argument.
       * printf("Now: %s\n", v.get<CHBString>()->getBuffer());   // access the newly created string
       * @endcode
       */
      template<typename T, typename InplacerT>
      void reset(InplacerT arg);

      /**
       * Destruct the stored object (if any) and default construct a new one of given type T.
       */
      template<typename T>
      void reset();

      /**
       * Accessor function.
       *
       * @return a pointer to the stored type. If a type is requested that is currently not stored in the variant, NULL is returned.
       */
      template<typename T>
      T* get();

      /**
       * Const accessor function.
       *
       * @return a pointer to the stored type. If a type is requested that is currently not stored in the variant, NULL is returned.
       */
      template<typename T>
      const T* get() const;

      /**
       * Check if the variant is unset.
       *
       * @return true if the variant does not contain a value, else false.
       */
      inline
      bool isEmpty() const
      {
         return mIdx == unset;
      }

      /**
       * Get the internal identifier for the stored type.
       *
       * @return the internal type identifier (which corresponds to the position of the stored type within the typelist typedef + 1)
       *         or 0 if the variant is unset.
       */
      inline
      signed char getTypeId() const
      {
         return static_cast<signed char>(mIdx);
      }


   private:

      ///@internal
      void try_destroy();

      ///@internal
      void construct_from(const TVariant& v);

      /// index of the current data type
      /// for DSI streaming this must be the first attribute in the class!
      int mIdx;

      /// with an ordinary union only simple data types could be stored in here
      typename std::tr1::aligned_storage<size, alignment>::type mData;
   };


// ------------------------------------------------------------------------------------------------


   template<typename TypelistT>
   template<typename T>
   inline
   TVariant<TypelistT>::TVariant(const T& t)
      : mIdx(typeIdOf<T>::value)
   {
      static_assert(Private::TypeListAlgos::Size<tTypelistType>::value > 1, "");//, a_variant_must_contain_at_least_two_elements);
      static_assert(Private::TypeListAlgos::Find<T, tTypelistType>::value >= 0, "");//, given_type_is_not_element_of_variant_maybe_use_explicit_cast);

      ::new(&mData) T(t);
   }


   template<typename TypelistT>
   template<typename T>
   TVariant<TypelistT>&
   TVariant<TypelistT>::operator=(const T& t)
   {
      static_assert(Private::TypeListAlgos::Find<T, tTypelistType>::value >= 0, ""); // given_type_is_not_element_of_variant_maybe_use_explicit_cast

      if (mIdx == typeIdOf<T>::value)
      {
         *get<T>() = t;
      }
      else
      {
         try_destroy();
         mIdx = typeIdOf<T>::value;
         ::new(&mData) T(t);
      }

      return *this;
   }


   template<typename TypelistT>
   TVariant<TypelistT>&
   TVariant<TypelistT>::operator=(const TVariant& v)
   {
      if (&v != this)
      {
         try_destroy();
         construct_from(v);
      }
      return *this;
   }


   template<typename TypelistT>
   inline
   void TVariant<TypelistT>::reset()
   {
      try_destroy();
      mIdx = unset;
   }


   template<typename TypelistT>
   template<typename T, typename InplacerT>
   inline
   void TVariant<TypelistT>::reset(InplacerT args)
   {
      try_destroy();

      static_assert(Private::TypeListAlgos::Size<tTypelistType>::value > 1, "");//, a_variant_must_contain_at_least_two_elements);
      static_assert(Private::TypeListAlgos::Find<T, tTypelistType>::value >= 0, "");//, given_type_is_not_element_of_variant_maybe_use_explicit_cast);

      mIdx = typeIdOf<T>::value;
      args.template apply<T>(&mData);
   }


   template<typename TypelistT>
   template<typename T>
   inline
   void TVariant<TypelistT>::reset()
   {
      try_destroy();

      static_assert(Private::TypeListAlgos::Size<tTypelistType>::value > 1, "");//, a_variant_must_contain_at_least_two_elements);
      static_assert(Private::TypeListAlgos::Find<T, tTypelistType>::value >= 0, "");//, given_type_is_not_element_of_variant_maybe_use_explicit_cast);

      mIdx = typeIdOf<T>::value;
      ::new(&mData) T();
   }


   template<typename TypelistT>
   template<typename T>
   inline
   T* TVariant<TypelistT>::get()
   {
      static_assert(Private::TypeListAlgos::Find<T, tTypelistType>::value >= 0, "");//, given_type_is_not_element_of_variant_maybe_use_explicit_cast);

      if (typeIdOf<T>::value != mIdx)
         return nullptr;

      return const_cast<T*>(reinterpret_cast<const T*>(&mData));
   }


   template<typename TypelistT>
   template<typename T>
   inline
   const T* TVariant<TypelistT>::get() const
   {
      static_assert(Private::TypeListAlgos::Find<T, tTypelistType>::value >= 0, "");//, given_type_is_not_element_of_variant_maybe_use_explicit_cast);

      if (typeIdOf<T>::value != mIdx)
         return nullptr;

      return const_cast<T*>(reinterpret_cast<const T*>(&mData));
   }


   template<typename TypelistT>
   void TVariant<TypelistT>::try_destroy()
   {
      if (mIdx > 0 && mIdx < Private::TypeListAlgos::Size<typename TVariant<tTypelistType>::tTypelistType>::value + 1)
         tHelperType::destruct(mIdx, &mData);
   }


   template<typename TypelistT>
   void TVariant<TypelistT>::construct_from(const TVariant& v)
   {
      mIdx = v.mIdx;

      if (mIdx > 0 && mIdx < Private::TypeListAlgos::Size<typename TVariant<tTypelistType>::tTypelistType>::value + 1)
         tHelperType::construct(mIdx, &mData, &v.mData);
   }


// ---------------------------------------------------------------------------------------------------------


/**
 * @class TStaticVisitor TVariant.hpp "dsi/TVariant.hpp"
 *
 * The static visitor can be used as baseclass for any other objects that should be used to visit
 * a variant object. The derived visitor must implement a corresponding function call operator for
 * any stored type within the variant. An empty function call operator must be provided to handle
 * empty variants.
 *
 * The function call operators may take non-const references when visiting a non-const variant. Thus, you may
 * modify the variant contents within a visitor. But make sure the signature does match the const modifier
 * of the variant, e.g. visiting a constant variant with a visitor taking non-const references will lead
 * to compilation problems.
 *
 * The following example creates a visitor which will retrieve the
 * integer value or the length of the stored string within the variant:
 *
 * @code
 * TVariant<int, CHBString> myVar;
 * ...
 * class CMyVisitor : public TStaticVisitor<int>
 * {
 * public:
 *    inline int operator()(int i) const { return i; }
 *    inline int operator()(const CHBString& str) const { return str.getLength(); }
 *
 *    inline int operator()() { ... }     ///< the empty variant switch
 * };
 *
 * ...
 * printf("The desired value is %d\n", staticVisit(CMyVisitor(), myVar));
 * @endcode
 */
   template<typename ReturnT = void>
   class TStaticVisitor
   {
   public:

      typedef ReturnT return_type;

   protected:

      ~TStaticVisitor()
      {
         // NOOP
      }
   };


/**
 * Visit a TVariant with the given visitor object. For an example see TStaticVisitor.
 *
 * @param visitor The visitor to use for visitation.
 * @param variant The variant to be visited.
 * @return the return value of the appropriate function call operator of the visitor.
 */
   template<typename VisitorT, typename VariantT>
   inline
   typename VisitorT::return_type staticVisit(VisitorT& visitor, VariantT& variant)
   {
      return VariantT::tHelperType::eval(variant.getTypeId(), visitor, variant);
   }


// ------------------------------------- equality ---------------------------------------


/// @internal
namespace Private
{

/// @internal
template<typename VariantT>
class CEqualityVisitor : public TStaticVisitor<bool>
{
public:

   inline explicit
   CEqualityVisitor(const VariantT& var)
      : mVar(var)
   {
      // NOOP
   }

   inline
   bool operator()()
   {
      // never reached
      assert(false);
      return false;
   }

   template<typename T>
   inline
   bool operator()(const T& t)
   {
      const T* val = mVar.template get<T>();
      return val != nullptr && *val == t;
   }


private:

   const VariantT& mVar;
};

}   // end namespace


   template<typename TypelistT>
   inline
   bool TVariant<TypelistT>::operator==(const TVariant& rhs) const
   {
      Private::CEqualityVisitor<TVariant<TypelistT> > v(*this);
      return mIdx == rhs.mIdx && (mIdx == 0 || staticVisit(v, rhs));
   }
}


#endif   // DSI_TVARIANT_HPP
