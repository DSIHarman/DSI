#!/bin/sh
if [ -z $1 ]; then
   BUILD="release"
else
   BUILD="$1"
fi

if [ ! -d /var/run/servicebroker ]; then
  mkdir /var/run/servicebroker
fi
killall servicebroker 2> /dev/null
killall test_unittests 2> /dev/null
sleep 1
../../build/${BUILD}/tests/unittests/test_unittests
