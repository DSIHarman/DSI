/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/

package com.harmanbecker.generate;

@SuppressWarnings("serial")
public class GenerateException extends Exception
{
   public GenerateException( String message, Exception ex )
   {
      super( message, ex );
   }

   public String getMessage()
   {
      String msg = super.getMessage();
      if( null != getCause())
         msg += "\n[" + getCause().getMessage() + "]";
      return msg ;
   }
}
