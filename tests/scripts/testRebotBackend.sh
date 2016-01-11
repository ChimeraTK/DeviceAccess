#! /bin/bash

../bin/RebotDummyServer -m ./mtcadummy.map&
SERVER_PID=$!
sleep .1

# run the test now; mskrebot uses ip address of the server in the dmap file 
../bin/testRebotDevice mskrebot ./dummies.dmap
TEST_RESULT=$? 

if [ $TEST_RESULT -ne 0 ] ; then # The above test failed; so do not proceed 
                                 # further and return 
 kill $SERVER_PID
 exit $TEST_RESULT
fi 

# mskrebot1 uses hostname of the server in the dmap file 
../bin/testRebotDevice mskrebot1 ./dummies.dmap
TEST_RESULT=$? 

 kill $SERVER_PID
 exit $TEST_RESULT