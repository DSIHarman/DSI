/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/

package com.harmanbecker.generate;

import java.io.IOException;
import java.io.PrintStream;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Vector;

import com.harmanbecker.dsigen.Debug;

/**
 * GenerateContext is responsible for loading a jet generated class,
 * generate code from int and save it to a file. The user can use the
 * set/getParameter methods for passing data into the jet template.
 */
public class GenerateContext
{
   private static final String GENERATE_FILENAME = "Generate.Filename" ;
   private static final String GENERATE_OUPUTDIR = "Generate.OutputDir" ;

   private HashMap<String, Object> mParameterMap = new HashMap<String, Object>();
   private Vector<GenerateFile> mFileOutput = new Vector<GenerateFile>();
   private long mSourceLastModified = Long.MAX_VALUE ;

   public void setSourceLastModified( long lastModified )
   {
      mSourceLastModified = lastModified ;
   }

   /**
    * Sets a generic object parameter
    *
    * @param name the name of the parameter
    * @param value the parameter value as an object reference
    */
   public void setObjectParameter( String name, Object value )
   {
      mParameterMap.put( name, value );
   }

   /**
    * Returns a generic object parameter
    *
    * @param name the name of the parameter
    * @return a reference to the object
    * @throws GenerateException if the parameter does not exist
    */
   public Object getObjectParameter( String name ) throws GenerateException
   {
      if( !mParameterMap.containsKey( name ) )
         throw new GenerateException("Unknown Generate Variable: " + name, null );
      return mParameterMap.get( name ) ;
   }

   /**
    * Sets a String parameter
    *
    * @param name the name of the parameter
    * @param value the parameter value
    */
   public void setStringParameter( String name, String value )
   {
      setObjectParameter( name, value );
   }

   /**
    * Returns a String parameter
    *
    * @param name the name of the parameter
    * @return the parameter value as a String
    * @throws GenerateException if the parameter does not exist
    */
   public String getStringParameter( String name ) throws GenerateException
   {
      Object object = getObjectParameter( name ) ;
      return null != object ? object.toString() : null ;
   }

   /**
    * Set an integer parameter
    *
    * @param name the name of the parameter
    * @param value the parameter value
    */
   public void setIntegerParameter( String name, int value )
   {
      setObjectParameter( name, new Integer(value) );
   }

   /**
    * Returns an integer parameter
    *
    * @param name the name of the parameter
    * @return the parameter value
    * @throws GenerateException if the parameter does not exist or
    *         if the parameter is not of type Integer
    */
   public int getIntegerParameter( String name ) throws GenerateException
   {
      try
      {
         return (Integer)getObjectParameter( name );
      }
      catch( ClassCastException ex )
      {
         throw new GenerateException("Bad Number Format (Variable: " + name + ")", ex );
      }
   }

   /**
    * Set a boolean parameter
    *
    * @param name the name of the parameter
    * @param value the parameter value
    */
   public void setBooleanParameter( String name, boolean value )
   {
      setObjectParameter( name, new Boolean(value) );
   }

   /**
    * Returns a boolean parameter
    *
    * @param name the name of the parameter
    * @return the parameter value
    * @throws GenerateException if the parameter does not exist or
    *         if the parameter is not of type Boolean
    */
   public boolean getBooleanParameter( String name ) throws GenerateException
   {
      Object object = null ;
      try
      {
         object = getObjectParameter( name );
         return ((Boolean)object).booleanValue();
      }
      catch( ClassCastException ex )
      {
         throw new GenerateException( "Bad Number Format (Variable: " + name
                                    + ", type " + ( null != object ? object.getClass().getName() : "null" ) + ")", ex );
      }
   }

   public void removeParameter( String name )
   {
      mParameterMap.remove( name );
   }

   /**
    * Sets the name of the template that will be used to generate the code.
    * The name is simply the class name of the template and the class must
    * be in a package called "template"
    *
    * @param name the class name of the class created by jet
    */
   public void setTemplateName( String name )
   {
      setStringParameter( GENERATE_FILENAME, name );
   }

   /**
    * Returns the name of the template used to create code.
    *
    *  @return The name of the template
    */
   public String getTemplateName() throws GenerateException
   {
      return getStringParameter( GENERATE_FILENAME );
   }

   /**
    * Sets the filename of the output file in which the created
    * code is places.
    *
    * @param filename the filename of the output file
    */
   public void setOutputDirectory( String filename )
   {
      setStringParameter( GENERATE_OUPUTDIR, filename );
   }

   /**
    * Returns the output filename.
    *
    * @return the output filename
    * @throws GenerateException if the filename has not been set before
    */
   public String getOutputDirectory() throws GenerateException
   {
      return getStringParameter( GENERATE_OUPUTDIR );
   }

   /**
    * Generates Code from a template and saves it to a file.
    *
    * @throws GenerateException if an error occurs during generation
    */
   public void generate() throws GenerateException
   {
      try
      {
         // Use introspection to load the class (class loader in java)
         Class<?> jetClass = Class.forName( getTemplateName() );
         // Create an instance of this class (always cast to base class)
         GenerateEmiterBase template = (GenerateEmiterBase)jetClass.newInstance();

         template.setSourceLastModified( mSourceLastModified );

         do
         {
            try
            {
               // Generate the file content
               String generatedContent = template.generate( this );

               // if generate() returns null we do not generate a file (it's a feature)
               if( null != generatedContent )
               {
                  // get the filename
                  String filename = template.getFilename() ;
                  if( null == filename || 0 == filename.length() )
                     throw new GenerateException("Filename not set for template \"" + getTemplateName() + "\"", null );
                  // create destination folder if needed
                  GenerateFile file = new GenerateFile( template.getOutputDirectory() + "/" + filename, generatedContent );

                  // only write if file needs to be written
                  mFileOutput.add( file );
               }
            }
            catch( FileNotGeneratedException e )
            {
            }
            // generate multiple files from one template?
         } while( template.again() );
      }
      catch( Exception ex )
      {
         if(!(ex instanceof GenerateException))
         {
            ex = new GenerateException( "Error generating from template \"" + getTemplateName() + "\"", ex );
         }
         throw (GenerateException)ex ;
      }
   }

   /**
    * Writes all generated files to disk.
    */
   public void writeOutputFiles() throws IOException
   {
      Enumeration<GenerateFile> e = mFileOutput.elements();
      while( e.hasMoreElements() )
      {
         GenerateFile file = e.nextElement() ;
         Debug.message("      " + file.getCanonicalPath() );
         file.write();
      }
      mFileOutput.clear();
   }

   /**
    * Generates Code from a template and saves it to a file.
    * @param templateName name of the template to use
    * @param outputFilename filename of the output filename
    * @throws GenerateException if an error occurs during generation
    */
   public void generate( String templateName ) throws GenerateException
   {
      if( null == templateName )
         throw new GenerateException("Generate: template name not set", null );

      setTemplateName( templateName );

      generate();
   }

   public void dump( PrintStream out )
   {
      out.println( "   Context Dump" );
      Iterator<String> keys = mParameterMap.keySet().iterator() ;
      while( keys.hasNext() )
      {
         String key = keys.next() ;
         out.println( "      " + key + " - " + mParameterMap.get( key ) );
      }
   }
}
