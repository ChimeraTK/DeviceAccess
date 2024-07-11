#!/bin/bash

TEST_RESULT=0
MAX_PORT_RETRY_COUNT=60

for PROTOCO_VERSION in 0 1; do
    #start the server with the protocol version to test against
    # -p 0 will make the server start on a random port. It will print the port it uses
    # on stdout, eg. "PORT 32442"
    output=`mktemp`
    ./RebotDummyServer -m ./mtcadummy_rebot.map -V $PROTOCO_VERSION -p 0 >$output &
    SERVER_PID=$!
    PORT_RETRY_COUNT=0
    PORT=""

    # We take the port from the server and modify the DMAP files accordingly.
    # It is passed on to the "subtests" as well as the server port itself
    while [ $PORT_RETRY_COUNT -le $MAX_PORT_RETRY_COUNT ] && [ -z $PORT ]; do
        sleep .5
        PORT=`awk '/^PORT /{print $2}' $output`
        let PORT_RETRY_COUNT+=1
    done

    if [ -z $PORT ] ; then
        echo "Failed to spawn RebotDummyServer within 30s. Expect failures below"
    fi

    DMAP=`mktemp -p . -t XXXXXXXXX.dmap`
    echo "Using server port $PORT"
    echo "Using dmap file $DMAP"

    sed -e "s|=5001|=$PORT|g" dummies.dmap > "$DMAP"

    # run the test now; mskrebot uses ip address of the server in the dmap file
    ./testRebotBackend mskrebot "$DMAP" "$PORT"
    if [ $? -ne 0 ] ; then # The above test failed;
        echo "Testing backed using the IP addess with protocol version ${PROTOCO_VERSION} failed!"
        TEST_RESULT=$(( ( $TEST_RESULT  + 1 ) + ( ${PROTOCO_VERSION} *10 ) ))
    fi

    # mskrebot1 uses hostname of the server in the dmap file
    ./testRebotBackend mskrebot1 "$DMAP" "$PORT"
    if [ $? -ne 0 ] ; then # The above test failed;
        echo "Testing backed using the hostname with protocol version ${PROTOCO_VERSION} failed!"
        TEST_RESULT=$(( ( $TEST_RESULT  + 2 ) + ( ${PROTOCO_VERSION} *10 ) ))
    fi

    ./testRebotBackendCreation "$DMAP" "$PORT"
    if [ $? -ne 0 ] ; then # The above test failed;
        echo "Testing RebotBackend creation failed!"
        TEST_RESULT=$(( $TEST_RESULT  + 4 ))
    fi

    rm -f "$DMAP" "$output"

    kill $SERVER_PID
    #a small sleep so the port is actually freed for the next server which is
    #starting in the next loop iteration
    sleep .1

done

exit $TEST_RESULT
