/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/

package com.harmanbecker.generate;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;

public class GenerateFile extends File
{
   private String mContent ;
   public GenerateFile( String filename, String content ) throws IOException
   {
      super( filename );
      mContent = content ;
   }

   /*
    * Writes the content of the generation to the file
    */
   public void write() throws IOException
   {
      // we give writing multiple chances. Sometimes we get
      // a "permission denied" exception.
      int retryCounter = 4 ;
      while( true )
      {
         try
         {
            if( exists() )
            {
               delete();
            }
            else
            {
               getParentFile().mkdirs();
            }
            // Create the file using a buffered writer
            FileWriter fileWriter = new FileWriter( this );
            fileWriter.write( mContent );
            fileWriter.close();

            // file successfully written -> end the loop
            break;
         }
         catch( IOException ex )
         {
            if( retryCounter-- <= 0 )
            {
               // we got 4 exceptions in a row
               throw ex ;
            }
         }
      }
   }
}
