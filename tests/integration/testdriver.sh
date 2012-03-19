#!/bin/sh

EXE=$1

if [ -z "$EXE" ]; then
   echo "Missing argument for executable to run. Must exit." >&2
   exit 1
fi

export DSI_SERVICEBROKER=/testsb
export SB_HTTP_PORT=9956

../../src/servicebroker/servicebroker -vvvvv -c -d -p $DSI_SERVICEBROKER &
SB_PID=$!

# busy loop until mountpoint is available
while [ ! -e /var/run/servicebroker/$DSI_SERVICEBROKER ]; do
   sleep 0.2
done
sleep 0.2

./$EXE
RC=$?

kill $SB_PID

exit $RC
