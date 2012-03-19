#!/bin/sh
if [ ! -d /var/run/servicebroker ]; then
  mkdir /var/run/servicebroker
fi
if [ $# -lt 1 ]; then
   echo specify the number of the step to be run
   exit 1
fi
step=step$1
killall servicebroker 2> /dev/null
killall -r server_step.* 2> /dev/null
killall -r client_step.* 2> /dev/null
`dirname $0`/../../src/servicebroker/servicebroker &
sleep 1
`dirname $0`/$step/server_$step $2 &
`dirname $0`/$step/client_$step $2
