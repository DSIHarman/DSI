/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/

package com.harmanbecker.dsigen;

import java.io.File;
import java.io.FileOutputStream;
import java.io.PrintStream;
import java.util.Enumeration;
import java.util.Vector;

import com.harmanbecker.generate.GenerateContext;

@SuppressWarnings("serial")
public class Main
{
   public static final int MAJOR_VERSION = 2 ;
   public static final int MINOR_VERSION = 0 ;

   private static Vector<String> mExtraIncludes = new Vector<String>() ;


   public static void main( String[] args )
   {
      int returnValue = -1 ;
      boolean forceGenerate = false ;
      boolean generateDefines = true ;
      boolean generateDateInformation = true ;
      String outputDirectory = null ;
      String outputDirectoryAS = null ;
      String generateDirectory = null ;
      String rootDirectory = null ;
      String inputFilename = null ;

      try
      {
         // parse the command line options
         for( int index=0; index<args.length; index++ )
         {
            String arg = args[index];
            if( arg.equals("-v") )
            {
               Debug.set( true );
            }
            else if( arg.equals("-f") )
            {
               forceGenerate = true ;
            }
            else if( arg.startsWith("--no-define") || arg.startsWith("-nd") )
            {
               generateDefines = false ;
            }
            else if( arg.equals("-nodate") )
            {
            	generateDateInformation = false ;
            }
            else if( arg.startsWith("-asdir=") )
            {
               outputDirectoryAS = arg.substring(7);
            }
            else if( arg.startsWith("-gendir=") )
            {
               generateDirectory = arg.substring(8);
            }
            else if( arg.startsWith("-rootdir=") )
            {
               rootDirectory = arg.substring(9);
            }
            else if( arg.startsWith("-include=") )
            {
               String include = arg.substring(9);
               if( 0 != include.length() )
               {
                  mExtraIncludes.add( include.replace('\\', '/') );
               }
            }
            else if( arg.startsWith("-sp=") )
            {
               SearchPath.add( arg.substring(4) );
            }
            else if( null == inputFilename && '-' != arg.charAt(0) )
            {
               inputFilename = arg ;
            }
            else if( null == outputDirectory && '-' != arg.charAt(0) )
            {
               outputDirectory = arg.replace('\\', '/') ;
            }
            else
            {
               inputFilename = outputDirectory = null ;
               break;
            }
         }

         if( Debug.get() )
         {
            Debug.message(Main.class.getName() + " " + MAJOR_VERSION + "." + MINOR_VERSION);
            Debug.message("   Command Line Arguments:" );
            for( int index=0; index<args.length; index++ )
            {
               Debug.message("      " + args[index] );
            }
         }

         if( null == inputFilename || null == outputDirectory )
         {
            System.err.println( "Bad parameters:");
            System.err.println( Main.class.getName() + ": [-v] [-sp=<search path>] (<hbsi file>|<hbtd file>) <output directory>");
            System.err.println( "-v              Verbose Mode" );
            System.err.println( "-f              Force generating of all output files" );
            System.err.println( "-nc             Do not generate *Client.h and *Client.c files" );
            System.err.println( "-nd             Do not generate defines for attribute streaming in header file" );
            System.err.println( "-nodate         Do not generate date indormation in the header of the files" );
            System.err.println( "-sp=<path>      Add search path for hbtd files" );
            System.err.println( "-gendir=<path>  Path of the gen products dir. Needed for abdtract interfaces" );
            System.err.println( "-rootdir=<path> Project Root Directory. Needed for abstract interfaces" );
         }
         else
         {
            Vector<String> templates = new Vector<String>()
            {
               public boolean add(String o)
               {
                  // avoid adding a template twice
                  return this.contains(o) ? true : super.add( o ) ;
               }
            };

            boolean hbsi = inputFilename.toLowerCase().endsWith(".hbsi");

            templates.add( "templates.DSI_Abstract_HPP" ) ;
            templates.add( "templates.DSI_Streaming_Abstract_HPP" ) ;
            templates.add( "templates.DSI_Streaming_HPP" ) ;
            templates.add( "templates.DSI_Streaming_CPP" ) ;
            templates.add( "templates.DSI_HPP" ) ;
            templates.add( "templates.DSI_CPP" ) ;
            if( hbsi )
            {
            	templates.add( "templates.CDSIProxy_Abstract_HPP" ) ;
            	templates.add( "templates.CDSIProxy_HPP" ) ;
            	templates.add( "templates.CDSIProxy_CPP" ) ;

            	templates.add( "templates.CDSIStub_Abstract_HPP" ) ;
            	templates.add( "templates.CDSIStub_HPP" ) ;
            	templates.add( "templates.CDSIStub_CPP" ) ;
            }

            Debug.message("   Output Directory" );
            Debug.message("      " + outputDirectory );

            inputFilename = new File( inputFilename ).getCanonicalPath();

            if( null != generateDirectory )
            {
               generateDirectory = new File( generateDirectory ).getCanonicalPath();
            }

            if( null != rootDirectory )
            {
               rootDirectory = new File( rootDirectory ).getCanonicalPath();
            }

            ServiceInterface si = new ServiceInterface( inputFilename, false );

            if( si.hasBaseInterfaces() )
            {
               String include = null ;
               if(null != generateDirectory && inputFilename.startsWith(generateDirectory))
               {
                  include = inputFilename.substring(1+generateDirectory.length()).replaceAll("\\.[^\\.]+$", "").replace('\\', '/');
               }
               else if(inputFilename.startsWith(rootDirectory))
               {
                  include = inputFilename.substring(1+rootDirectory.length()).replaceAll("\\.[^\\.]+$", "").replace('\\', '/');
               }
               else
               {
                  Debug.error( inputFilename + " not under project or generate directory" );
               }

               si.setIncludeName(include);
            }

            GenerateContext context = new GenerateContext();

            if( !forceGenerate )
            {
               File jarFile = new File(Main.class.getProtectionDomain().getCodeSource().getLocation().toURI());
               if( jarFile.getName().endsWith(".jar"))
               {
                  context.setSourceLastModified(Math.max(jarFile.lastModified(), ServiceInterface.lastModified()));
               }
            }
            context.setObjectParameter("ServiceInterface", si);
            context.setObjectParameter("ExtraIncludes", mExtraIncludes.toArray(new String[mExtraIncludes.size()]));
            context.setBooleanParameter("generateDefines", generateDefines );
            context.setBooleanParameter("generateDateInformation", generateDateInformation );
            context.setBooleanParameter("ishbtd", !hbsi );
            context.setOutputDirectory(outputDirectory);
            context.setStringParameter("generateDirectory", generateDirectory );
            context.setStringParameter("rootDirectory", rootDirectory );
            context.setStringParameter("ASOutputDir", outputDirectoryAS );
            context.setStringParameter( "version", MAJOR_VERSION + "." + MINOR_VERSION );

            if( Debug.get() )
            {
               context.dump(Debug.getDebugStream());
            }

            Enumeration<String> e = templates.elements();
            while( e.hasMoreElements() )
            {
               String template = e.nextElement();
               context.generate( template );
            }

            Debug.message("   Writing output files");
            context.writeOutputFiles();

            Debug.message("   Done.\n");
            returnValue = 0 ;
         }
      }
      catch( DSIException ex )
      {
         if( ex.isNoDSIInterfaceError() )
         {
            String dummyFile =  outputDirectory + "/DSI"  + Util.getBaseName( inputFilename ) + "Stream.cpp" ;

            Debug.message( "   Input file is not extern. Stoping generation." );
            Debug.message( "   Creating Dummy File." );
            Debug.message( "      " + dummyFile );
            try
            {
               PrintStream ps = new PrintStream( new FileOutputStream( dummyFile ));
               ps.println("");
               ps.println("// dummy file to satisfy compiler.");
               ps.println("");
               ps.close();
               returnValue = 0 ;
            }
            catch( Exception ee )
            {
               Debug.error( ee );
            }
         }
         else
         {
            Debug.error( ex );
         }
      }
      catch( Exception ex )
      {
         Debug.error( ex );
      }

      // terminate JVM with appropriate return value
      Runtime.getRuntime().exit( returnValue );
   }
}
