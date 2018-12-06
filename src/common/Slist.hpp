/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_COMMON_SLIST_HPP
#define DSI_COMMON_SLIST_HPP

#include <cassert>
#include <iterator>
#include <tr1/type_traits>

#include "dsi/private/if.hpp"
#include "dsi/private/CNonCopyable.hpp"


namespace DSI
{

   /**
    * @ingroup dsi_containers
    *
    * Intrusive containers make assumptions on the contained data type.
    */
   namespace intrusive
   {
      // FIXME remove this
      template<typename IteratorT>
      size_t distance(IteratorT first, IteratorT last)
      {
         size_t ret = 0;

         while(first != last)
         {
            ++first;
            ++ret;
         }

         return ret;
      }

      /**
       * @internal
       */
      template<typename DataT>
      struct SlistIteratorTraits
      {
         typedef DataT& tReferenceType;
         typedef DataT* tPointerType;
      };


      /**
       * @internal
       */
      template<typename DataT>
      struct SlistIteratorTraits<const DataT>
      {
         typedef const DataT& tReferenceType;
         typedef const DataT* tPointerType;
      };


      /**
       * This list hook structure acts as a base class for an intrusive list entry.
       *
       * @code
       * struct SHBMyStruct : public intrusive::SlistBaseHook
       * {
       *    ...
       * };
       * @endcode
       */
      struct SlistBaseHook
      {
         inline
         SlistBaseHook()
            : mNext(nullptr)
         {
            // NOOP
         }

         SlistBaseHook* mNext;
      };

      /**
       * This list hook structure acts as a member for an intrusive list entry.
       *
       * @code
       * struct SHBMyStruct
       * {
       *    intrusive::SlistMemberHook mTheHook;
       *    ...
       * };
       * @endcode
       *
       * A list entry can have multiple member hooks and thus can be hooked simultaneously into
       * multiple lists.
       */
      struct SlistMemberHook
      {
         void* mNext;
      };


      /**
       * Policy to access the single linked list entry via the base class of the entry.
       */
      template<typename DataT>
      struct SlistBaseHookAccessor
      {
         DataT* next(const SlistBaseHook* d) const;
         void hookAfter(SlistBaseHook* element, SlistBaseHook* after);
         void unhookNext(SlistBaseHook* d);
      };


      template<typename DataT>
      inline
      DataT* SlistBaseHookAccessor<DataT>::next(const SlistBaseHook* d) const
      {
         return static_cast<DataT*>(d->mNext);
      }


      template<typename DataT>
      inline
      void SlistBaseHookAccessor<DataT>::hookAfter(SlistBaseHook* element, SlistBaseHook* after)
      {
         element->mNext = after->mNext;
         after->mNext = element;
      }


      template<typename DataT>
      inline
      void SlistBaseHookAccessor<DataT>::unhookNext(SlistBaseHook* d)
      {
         SlistBaseHook* to_unhook = next(d);
         d->mNext = next(to_unhook);
         to_unhook->mNext = nullptr;
      }


      /**
       * Policy to access the single linked list entry via a data member of the entry.
       *
       * @todo could use type traits to get DataT from MemberPtr?
       */
      template<typename DataT, struct SlistMemberHook DataT::* MemberPtr>
      struct SlistMemberHookAccessor
      {
         DataT* next(const DataT* d) const;
         DataT* next(const SlistBaseHook* d) const;

         void hookAfter(DataT* element, DataT* after);
         void hookAfter(DataT* element, SlistBaseHook* after);

         void unhookNext(DataT* d);
         void unhookNext(SlistBaseHook* d);

      };


      template<typename DataT, struct SlistMemberHook DataT::* MemberPtr>
      inline
      DataT* SlistMemberHookAccessor<DataT, MemberPtr>::next(const DataT* d) const
      {
         return reinterpret_cast<DataT*>(((d)->*MemberPtr).mNext);
      }


      template<typename DataT, struct SlistMemberHook DataT::* MemberPtr>
      inline
      DataT* SlistMemberHookAccessor<DataT, MemberPtr>::next(const SlistBaseHook* d) const
      {
         return reinterpret_cast<DataT*>(d->mNext);
      }


      template<typename DataT, struct SlistMemberHook DataT::* MemberPtr>
      inline
      void SlistMemberHookAccessor<DataT, MemberPtr>::hookAfter(DataT* element, DataT* after)
      {
         (element->*MemberPtr).mNext = next(after);
         (after->*MemberPtr).mNext = element;
      }


      template<typename DataT, struct SlistMemberHook DataT::* MemberPtr>
      inline
      void SlistMemberHookAccessor<DataT, MemberPtr>::hookAfter(DataT* element, SlistBaseHook* after)
      {
         (element->*MemberPtr).mNext = after->mNext;
         after->mNext = reinterpret_cast<SlistBaseHook*>(element);
      }


      template<typename DataT, struct SlistMemberHook DataT::* MemberPtr>
      inline
      void SlistMemberHookAccessor<DataT, MemberPtr>::unhookNext(DataT* d)
      {
         DataT* to_unhook = next(d);
         (d->*MemberPtr).mNext = next(to_unhook);
         (to_unhook->*MemberPtr).mNext = nullptr;
      }


      template<typename DataT, struct SlistMemberHook DataT::* MemberPtr>
      inline
      void SlistMemberHookAccessor<DataT, MemberPtr>::unhookNext(SlistBaseHook* d)
      {
         DataT* to_unhook = next(d);
         d->mNext = reinterpret_cast<SlistBaseHook*>(next(to_unhook));
         (to_unhook->*MemberPtr).mNext = nullptr;
      }


      struct StaticCaster
      {
         template<typename T, typename U>
         static inline
         T safe_cast(U u)
         {
            return static_cast<T>(u);
         }
      };


      struct ReinterpretCaster
      {
         template<typename T, typename U>
         static inline
         T safe_cast(U u)
         {
            return reinterpret_cast<T>(u);
         }
      };


      /**
       * @class Slist Slist.hpp "common/Slist.hpp"
       *
       * Intrusive single linked list template. An intrusive container makes assumptions on the stored data. In the case of the single linked list
       * the stored data type must contain pointers the the next element to be linked. There are two possibilities of hooking the elements: either
       * by accessing the stored data's base class or by accessing its members. The latter is especially useful when elements should be stored in
       * multiple lists since each list must have its own individual hook member.
       *
       * Intrusive containers do not copy the elements pushed into the container (but takes the ownership). They do not allocate any memory.
       * Erasing elements using the member function @c erase does not delete the appropriate memory.
       * Anyhow, the member functions @c erase_and_dispose et al. allow to remove elements and call a disposer function (e.g. operator delete) to
       * free the used system resources.
       *
       * @code
       * struct MyData : public intrusive::SlistBaseHook
       * {
       *    ...
       *
       *    static void dispose(SHBMyData* data)
       *    {
       *       delete data;
       *    }
       * };
       *
       * // create empty list
       * Slist<SHBMyData> mylist;
       *
       * // create a stored element
       * SHBMyData* mydata = new SHBMyData();
       *
       * // append the element to the list
       * mylist.push_front(*mydata);
       *
       * // remove the element from the list and free heap storage
       * mylist.erase_and_dispose(mylist.begin(), SHBMyData::dispose);
       * @endcode
       *
       * Concerning the syntax creating a list which makes use of the member hook accessor policy is a bit more complex:
       *
       * @code
       * struct SHBMyData
       * {
       *    ...
       *    intrusive::SlistMemberHook mHook;
       * };
       *
       * typedef Slist<SHBMyData, intrusive::SlistMemberHookAccessor<SHBMyData, &SHBMyData::mHook> tMyListType;
       * tMyListType mylist;
       * ...
       * @endcode
       *
       * The advantage of member hook accessors it that elements could have multiple hook members than thus can be hooked simultaneously into
       * multiple containers.
       */
      template<
         typename DataT,
         typename AccessorPolicyT = intrusive::SlistBaseHookAccessor<DataT> >
      class Slist : private AccessorPolicyT, public Private::CNonCopyable
      {
         typedef intrusive::SlistBaseHook tHookType;

         typedef typename if_<std::tr1::is_same<AccessorPolicyT, intrusive::SlistBaseHookAccessor<DataT> >::value,
                              intrusive::StaticCaster,
                              intrusive::ReinterpretCaster>::type
         tCastType;

      public:

         template<typename IteratorDataT>
         class SlistIterator : public intrusive::SlistIteratorTraits<IteratorDataT>, private AccessorPolicyT
         {
            friend class SlistIterator<const DataT>;
            friend class SlistIterator<DataT>;

         public:

            typedef typename intrusive::SlistIteratorTraits<IteratorDataT>::tReferenceType tReferenceType;
            typedef typename intrusive::SlistIteratorTraits<IteratorDataT>::tPointerType tPointerType;


            inline
            SlistIterator()
               : ptr_(nullptr)
            {
               // NOOP
            }

            inline
            SlistIterator(tPointerType ptr)
               : ptr_(ptr)
            {
               // NOOP
            }

            inline
            SlistIterator(const SlistIterator<DataT>& rhs)
               : ptr_(rhs.ptr_)
            {
               // NOOP
            }

            inline
            SlistIterator& operator++()
            {
               assert(ptr_);
               ptr_ = this->next(ptr_);
               return *this;
            }

            inline
            SlistIterator operator++(int)
            {
               assert(ptr_);
               SlistIterator iter(*this);
               ++(*this);
               return iter;
            }

            inline
            tPointerType operator->()
            {
               assert(ptr_);
               return get();
            }

            inline
            tPointerType get()
            {
               assert(ptr_);
               return static_cast<tPointerType>(ptr_);
            }

            inline
            tReferenceType operator*()
            {
               assert(ptr_);
               return *static_cast<tPointerType>(ptr_);
            }

            inline
            bool operator==(const SlistIterator<IteratorDataT>& rhs) const
            {
               return ptr_ == rhs.ptr_;
            }

            inline
            bool operator!=(const SlistIterator<IteratorDataT>& rhs) const
            {
               return ptr_ != rhs.ptr_;
            }

         private:
            tPointerType ptr_;
         };


         typedef DataT                         tValueType;
         typedef SlistIterator<DataT>          iterator;
         typedef SlistIterator<const DataT>    const_iterator;

      private:

         tHookType mFirst;

      public:

         /**
          * Default constructor. Creates an empty sequence.
          */
         Slist();

         /**
          * Destructor.
          */
         ~Slist();

         /**
          * @return an iterator to the beginning of the controlled sequence.
          */
         iterator begin();

         /**
          * @return a const iterator to the beginning of the controlled sequence.
          */
         const_iterator begin() const;

         /**
          * @return a past-the-end iterator for the controlled sequence.
          */
         iterator end();

         /**
          * @return a constant past-the-end iterator for the controlled sequence.
          */
         const_iterator end() const;

         /**
          * Inserts the data given by @c val at the beginning of the sequence.
          *
          * @param val The value to insert.
          */
         void push_front(DataT& val);

         /**
          * The member function removes the first element of the sequence. Must not be called on an empty sequence.
          */
         void pop_front();

         /**
          * @return a const reference to the first element in the sequence.
          * @note Must not be called on an empty sequence.
          */
         const tValueType& front() const;

         /**
          * @return a reference to the first element in the sequence.
          * @note Must not be called on an empty sequence.
          */
         tValueType& front();

         /**
          * The member function returns true for an empty controlled sequence.
          *
          * @return true if the controlled sequence is empty, else false.
          */
         bool empty() const;

         /**
          * The member function removes all elements from the controlled sequence.
          */
         void clear();

         /**
          * The member function removes all elements from the controlled sequence and disposes the data
          * by calling the disposer function.
          */
         template<typename DisposerT>
         void clear_and_dispose(DisposerT disposer);

         /**
          * The member function returns the length of the controlled sequence.
          *
          * @return the number of elements in the sequence.
          */
         size_t size() const;

         /**
          * Erase the given iterator position from the controlled sequence. @c end()
          * is a valid value and raises a critical error.
          *
          * @param where The position to erase from the controlled sequence.
          * @return the iterator after @c where.
          */
         iterator erase(iterator where);

         /**
          * Erase the given iterator position from the controlled sequence and dispose
          * the element with the given disposer function.
          *
          * @param where The position to erase from the controlled sequence.
          * @param  disposer Disposer function to call after erasing the element pointed to by @c where.
          * @return the iterator after @c where.
          */
         template<typename DisposerT>
         iterator erase_and_dispose(iterator where, DisposerT disposer);

         /**
          * Insert after @c where. @c end() is NOT a valid value.
          *
          * @param where The iterator position after which to insert.
          * @param data The value to insert.
          *
          * @return iterator to newly inserted element.
          */
         iterator insert_after(iterator where, tValueType& data);

         /**
          * Insert before @c where. @c end() is a valid value for the insert position.
          *
          * @param where The iterator position before which to insert.
          * @param data The value to insert.

          * @return iterator to newly inserted element.
          */
         iterator insert(iterator where, tValueType& data);

         /**
          * The member function removes from the controlled sequence all elements, designated by the iterator where, for which *where == val.
          *
          * @param val The value to compare with.
          */
         void remove(const tValueType& val);

         /**
          * The member function removes from the controlled sequence all elements, designated by the iterator where, for which *where == val.
          *
          * @param val The value to compare with.
          * @param disposer The disposer to be used to delete the contained elements.
          */
         template<typename DisposerT>
         void remove_and_dispose(const tValueType& val, DisposerT disposer);

         /**
          * The member function removes from the controlled sequence all elements, designated by the iterator where, for which pred(*where) is true.
          *
          * @param pred The predicate to use for comparison.
          */
         template<typename PredicateT>
         void remove_if(PredicateT pred);

         /**
          * The member function removes from the controlled sequence all elements, designated by the iterator where, for which pred(*where) is true.
          *
          * @param pred The predicate to use for comparison.
          */
         template<typename PredicateT, typename DisposerT>
         void remove_if_and_dispose(PredicateT pred, DisposerT disposer);

         /**
          * The member function reverses the order in which elements appear in the controlled sequence.
          */
         void reverse();
      };



      template<typename DataT, typename AccessorPolicyT>
      inline
      Slist<DataT, AccessorPolicyT>::Slist()
      {
         clear();
      }


      template<typename DataT, typename AccessorPolicyT>
      inline
      Slist<DataT, AccessorPolicyT>::~Slist()
      {
         // NOOP
      }


      template<typename DataT, typename AccessorPolicyT>
      inline
      typename Slist<DataT, AccessorPolicyT>::iterator Slist<DataT, AccessorPolicyT>::begin()
      {
         if (empty())
         {
            return iterator(tCastType::template safe_cast<DataT*>(&mFirst));
         }
         else
            return iterator(tCastType::template safe_cast<DataT*>(mFirst.mNext));
      }


      template<typename DataT, typename AccessorPolicyT>
      inline
      typename Slist<DataT, AccessorPolicyT>::const_iterator Slist<DataT, AccessorPolicyT>::begin() const
      {
         if (empty())
         {
            return const_iterator(tCastType::template safe_cast<const DataT*>(&mFirst));
         }
         else
         {
             return const_iterator(tCastType::template safe_cast<const DataT*>(mFirst.mNext));
         }
      }


      template<typename DataT, typename AccessorPolicyT>
      typename Slist<DataT, AccessorPolicyT>::iterator Slist<DataT, AccessorPolicyT>::end()
      {
         return iterator(tCastType::template safe_cast<DataT*>(&mFirst));
      }


      template<typename DataT, typename AccessorPolicyT>
      inline
      typename Slist<DataT, AccessorPolicyT>::const_iterator Slist<DataT, AccessorPolicyT>::end() const
      {
         return const_iterator(tCastType::template safe_cast<const DataT*>(&mFirst));
      }


      template<typename DataT, typename AccessorPolicyT>
      inline
      void Slist<DataT, AccessorPolicyT>::push_front(DataT& val)
      {
         this->hookAfter(&val, &mFirst);
      }


      template<typename DataT, typename AccessorPolicyT>
      inline
      void Slist<DataT, AccessorPolicyT>::pop_front()
      {
         assert(!empty());
         this->unhookNext(&mFirst);
      }


      template<typename DataT, typename AccessorPolicyT>
      inline
      const typename Slist<DataT, AccessorPolicyT>::tValueType& Slist<DataT, AccessorPolicyT>::front() const
      {
         assert(!empty());
         return *(this->next(&mFirst));
      }


      template<typename DataT, typename AccessorPolicyT>
      inline
      typename Slist<DataT, AccessorPolicyT>::tValueType& Slist<DataT, AccessorPolicyT>::front()
      {
         assert(!empty());
         return *(this->next(&mFirst));
      }


      template<typename DataT, typename AccessorPolicyT>
      inline
      bool Slist<DataT, AccessorPolicyT>::empty() const
      {
         return mFirst.mNext == &mFirst;
      }


      template<typename DataT, typename AccessorPolicyT>
      inline
      void Slist<DataT, AccessorPolicyT>::clear()
      {
         mFirst.mNext = &mFirst;
      }


      template<typename DataT, typename AccessorPolicyT>
      template<typename DisposerT>
      inline
      void Slist<DataT, AccessorPolicyT>::clear_and_dispose(DisposerT disposer)
      {
         while(size() > 0)
            erase_and_dispose(begin(), disposer);
      }


      template<typename DataT, typename AccessorPolicyT>
      inline
      size_t Slist<DataT, AccessorPolicyT>::size() const
      {
         // FIXME use std::distance but then iterator must be good enough declared
         return distance(begin(), end());
      }


      template<typename DataT, typename AccessorPolicyT>
      typename Slist<DataT, AccessorPolicyT>::iterator Slist<DataT, AccessorPolicyT>::erase(iterator where)
      {
         if (where == begin())
         {
            pop_front();
            return begin();
         }
         else
         {
            for (iterator iter = begin(); iter != end(); ++iter)
            {
               iterator tmp(iter);
               if (++tmp == where)
               {
                  this->unhookNext(iter.get());
                  return ++iter;
               }
            }

            // this is a programming mistake
            assert(false);
            return end();
         }
      }


      template<typename DataT, typename AccessorPolicyT>
      template<typename DisposerT>
      inline
      typename Slist<DataT, AccessorPolicyT>::iterator Slist<DataT, AccessorPolicyT>::erase_and_dispose(iterator where, DisposerT disposer)
      {
         iterator iter = erase(where);
         disposer(where.get());
         return iter;
      }


      template<typename DataT, typename AccessorPolicyT>
      inline
      typename Slist<DataT, AccessorPolicyT>::iterator Slist<DataT, AccessorPolicyT>::insert_after(iterator where, tValueType& data)
      {
         this->hookAfter(&data, where.get());
         return iterator(&data);
      }


      template<typename DataT, typename AccessorPolicyT>
      typename Slist<DataT, AccessorPolicyT>::iterator Slist<DataT, AccessorPolicyT>::insert(iterator where, tValueType& data)
      {
         if (where == begin())
         {
            push_front(data);
            return iterator(&data);
         }
         else
         {
            for (iterator iter = begin(); iter != end(); ++iter)
            {
               iterator tmp(iter);
               ++tmp;
               if (tmp == where || tmp == end())
               {
                  this->hookAfter(&data, iter.get());
                  return iterator(&data);
               }
            }

            // this is a programming mistake
            assert(false);
         }
      }


      template<typename DataT, typename AccessorPolicyT>
      void Slist<DataT, AccessorPolicyT>::remove(const tValueType& val)
      {
         for (iterator iter = begin(); iter != end(); )
         {
            if (*iter == val)
            {
               iter = erase(iter);
            }
            else
               ++iter;
         }
      }


      template<typename DataT, typename AccessorPolicyT>
      template<typename DisposerT>
      void Slist<DataT, AccessorPolicyT>::remove_and_dispose(const tValueType& val, DisposerT disposer)
      {
         for (iterator iter = begin(); iter != end(); )
         {
            if (*iter == val)
            {
               iter = erase_and_dispose(iter, disposer);
            }
            else
               ++iter;
         }
      }


      template<typename DataT, typename AccessorPolicyT>
      template<typename PredicateT>
      void Slist<DataT, AccessorPolicyT>::remove_if(PredicateT pred)
      {
         for (iterator iter = begin(); iter != end(); )
         {
            if (pred(*iter))
            {
               iter = erase(iter);
            }
            else
               ++iter;
         }
      }


      template<typename DataT, typename AccessorPolicyT>
      template<typename PredicateT, typename DisposerT>
      void Slist<DataT, AccessorPolicyT>::remove_if_and_dispose(PredicateT pred, DisposerT disposer)
      {
         for (iterator iter = begin(); iter != end(); )
         {
            if (pred(*iter))
            {
               iter = erase_and_dispose(iter, disposer);
            }
            else
               ++iter;
         }
      }


      template<typename DataT, typename AccessorPolicyT>
      void Slist<DataT, AccessorPolicyT>::reverse()
      {
         if (!empty())
         {
            // store original list head
            intrusive::SlistBaseHook list;
            list.mNext = mFirst.mNext;

            clear();

            // now iterate over 'old' list
            for (DataT* data = tCastType::template safe_cast<DataT*>(list.mNext); data != tCastType::template safe_cast<DataT*>(&mFirst); )
            {
               DataT* next = this->next(data);
               push_front(*data);
               data = next;
            }
         }
      }


   }  // namespace intrusive
}  // namespace DSI

#endif   // DSI_COMMON_SLIST_HPP
