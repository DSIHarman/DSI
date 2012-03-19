#!/bin/sh

BUILDTYPE=
MAKETARGET=


toLower() 
{
   echo $1 | tr "[:upper:]" "[:lower:]" 
}

setBuildType()
{
   if [ -z "$BUILDTYPE" ]; then
      BUILDTYPE=$1
   else
      echo "Error: multiple build type setting. Must exit." >&2
      exit 1
   fi   
}


# parse arguments
while [ $# -gt 0 ]; do   

   if [ "$1" = "--debug" ]; then
      setBuildType Debug      
   elif [ "$1" = "--release" ]; then      
      setBuildType Release         
   elif [ "`toLower $1`" = "debug" ]; then
      echo "Deprecated build type, use --debug instead." >&2
      setBuildType Debug
   elif [ "`toLower $1`" = "release" ]; then
      echo "Deprecated build type, use --release instead." >&2
      setBuildType Release
   else
      MAKETARGET="$MAKETARGET $1"
   fi
   
   shift
done


# set defaults if no argument is given
if [ -z "$BUILDTYPE" ]; then
   BUILDTYPE=Debug
fi

if [ -z "$MAKETARGET" ]; then
   MAKETARGET=all
fi

# add this for verbose test output on failure when running cmake with make target 'test'
if [ $MAKETARGET = "test" ]; then
   export CTEST_OUTPUT_ON_FAILURE=1
fi

BUILDDIR=build/`toLower $BUILDTYPE`

if ls src >/dev/null && ls include >/dev/null ; then

   mkdir -p $BUILDDIR

   if cd $BUILDDIR >/dev/null 2>&1 ; then

      if cmake -DCMAKE_BUILD_TYPE=$BUILDTYPE ../.. ; then

         make $MAKETARGET 
         exit 1

      else
         echo "cmake finished unsuccessfully. Must exit."
         exit 0
      fi

   else
      echo "Cannot change into build directory '$BUILDDIR'. Must exit."
      exit 0
   fi   

else
   echo "Please call this script in the toplevel source directory. Must exit."
   exit 0
fi
