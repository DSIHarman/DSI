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
killall server 2> /dev/null
killall client 2> /dev/null
../../build/${BUILD}/src/servicebroker/servicebroker &
sleep 1
../../build/${BUILD}/tests/benchmark/server &
../../build/${BUILD}/tests/benchmark/client --pings=70000
