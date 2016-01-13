#!/bin/bash

../bin/RebotDummyServer -m ./mtcadummy_rebot.map&
SERVER_PID=$!
sleep .1

TEST_RESULT=0

# run the test now; mskrebot uses ip address of the server in the dmap file 
../bin/testRebotDevice mskrebot ./dummies.dmap
if [ $? -ne 0 ] ; then # The above test failed;
 TEST_RESULT=1
fi 

# mskrebot1 uses hostname of the server in the dmap file 
../bin/testRebotDevice mskrebot1 ./dummies.dmap
if [ $? -ne 0 ] ; then # The above test failed;
 TEST_RESULT=$(( $TEST_RESULT  + 2 ))
fi 

 kill $SERVER_PID
 exit $TEST_RESULT
