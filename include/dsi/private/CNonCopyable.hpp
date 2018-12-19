/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_PRIVATE_CNONCOPYABLE_HPP
#define DSI_PRIVATE_CNONCOPYABLE_HPP


namespace DSI
{
   namespace Private
   {

      /**
       * @internal
       *
       * Baseclass/markerinterface for all noncopyable objects.
       */
      class CNonCopyable
      {
      protected:

         /**
          * This is a NOOP.
          */
         inline
         CNonCopyable()
         {
            // NOOP
         }

         /**
          * This is a NOOP.
          */
         inline
         ~CNonCopyable()
         {
            // NOOP
         }


      private:  /// emphasize the following members are private

         /**
          * Copies are not allowed for this kind of object.
          */
         CNonCopyable(const CNonCopyable&);

         /**
          * Copies are not allowed for this kind of object.
          */
         const CNonCopyable& operator=(const CNonCopyable&);
      };

   }   // namespace Private
} // namespace DSI


#endif  // DSI_PRIVATE_NONCOPYABLE_HPP
