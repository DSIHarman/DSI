#!/bin/sh

# remove socket files - if still existent
rm -f /var/run/servicebroker/master
rm -f /var/run/servicebroker/slave

# start master
export SB_HTTP_PORT=9956
export SB_MASTER_PORT=9957
../../src/servicebroker/servicebroker -vvvvv -c -d -p /master -t -f servicebroker.cfg &
SB1_PID=$!

# start slave
export SB_HTTP_PORT=9958
../../src/servicebroker/servicebroker -vvvvv -c -d -p /slave -m 127.0.0.1:9957 -f servicebroker.cfg &
SB2_PID=$!

# wait to get the servicebrokers ready (and connected to each other)
sleep 1

# run the test
./test_servicebroker
RC=$?

# stop servicebrokers again
kill $SB1_PID
kill $SB2_PID

# return exitcode of test
exit $RC
