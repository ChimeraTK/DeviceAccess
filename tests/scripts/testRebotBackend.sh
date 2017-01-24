#!/bin/bash

TEST_RESULT=0

for PROTOCO_VERSION in 0 1; do

    #start the server with the protocol version to test against
    ../bin/RebotDummyServer -m ./mtcadummy_rebot.map -V $PROTOCO_VERSION&
    SERVER_PID=$!
    sleep .1

    # run the test now; mskrebot uses ip address of the server in the dmap file 
    ../bin/testRebotBackend mskrebot ./dummies.dmap
    if [ $? -ne 0 ] ; then # The above test failed;
        echo "Testing backed using the IP addess with protocol version ${PROTOCO_VERSION} failed!"
        TEST_RESULT=$(( ( $TEST_RESULT  + 1 ) + ( ${PROTOCO_VERSION} *10 ) ))
    fi 

    # mskrebot1 uses hostname of the server in the dmap file 
    ../bin/testRebotBackend mskrebot1 ./dummies.dmap
    if [ $? -ne 0 ] ; then # The above test failed;
        echo "Testing backed using the hostname with protocol version ${PROTOCO_VERSION} failed!"
        TEST_RESULT=$(( ( $TEST_RESULT  + 2 ) + ( ${PROTOCO_VERSION} *10 ) ))
    fi 

    ../bin/testRebotBackendCreation
    if [ $? -ne 0 ] ; then # The above test failed;
        echo "Testing RebotBackend creation failed!"
        TEST_RESULT=$(( $TEST_RESULT  + 4 ))
    fi 

    kill $SERVER_PID

done


exit $TEST_RESULT
