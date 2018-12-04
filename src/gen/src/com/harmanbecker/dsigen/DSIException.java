/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/

package com.harmanbecker.dsigen;

@SuppressWarnings("serial")
public class DSIException extends Exception
{
   static public final int ERR_UNSET = 0 ;
   static public final int ERR_NO_DSI_INTERFACE = 1 ;

   private int mError ;

   public DSIException( String message )
   {
      super( message );
      mError = ERR_UNSET ;
   }

   public DSIException( String message, int error )
   {
      super( message );
      mError = error ;
   }

   public boolean isNoDSIInterfaceError()
   {
      return mError == ERR_NO_DSI_INTERFACE ;
   }
}
