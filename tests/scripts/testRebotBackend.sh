#! /bin/bash

../bin/RebotDummyServer -m ./mtcadummy.map&
SERVER_PID=$!
sleep .1

# run the test now
../bin/testRebotDevice mskrebot ./dummies.dmap
TEST_RESULT=$? 

kill $SERVER_PID
exit $TEST_RESULT