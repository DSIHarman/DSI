/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/

package com.harmanbecker.generate;

import java.io.File;


public abstract class GenerateEmiterBase
{
   abstract protected String doGenerate( GenerateContext context ) throws Exception ;

   public String generate( GenerateContext context ) throws Exception
   {
      mAgain = false ;
      mOutputDirectory = context.getOutputDirectory();
      return doGenerate( context );
   }

   /*
    * the name of the file where the output is written to
    */
   private String mFilename = null ;

   /*
    * returns the output filename
    */
   public String getFilename()
   {
      return mFilename ;
   }

   /*
    * sets the output filename
    */
   protected void OUTPUTFILE( String filename ) throws GenerateException
   {
      mFilename = filename ;

      File file = new File( mOutputDirectory + "/" + filename );
      if( file.exists() && (mSourceLastModified < file.lastModified()))
      {
         /*
          * no need to generate this file
          */
         throw new FileNotGeneratedException();
      }
   }

   private long mSourceLastModified = Long.MAX_VALUE ;

   public void setSourceLastModified( long lastModified )
   {
      mSourceLastModified = lastModified ;
   }

   /*
    * tells the generate if there are more files to generate
    */
   private boolean mAgain = false ;

   protected void GENAGAIN()
   {
      mAgain = true ;
   }

   public boolean again()
   {
      return mAgain ;
   }

   protected void OUTPUTDIR( String directory )
   {
      mOutputDirectory = directory ;
   }

   protected String mOutputDirectory ;
   public String getOutputDirectory()
   {
      return mOutputDirectory ;
   }
}
