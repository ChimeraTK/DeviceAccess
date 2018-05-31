#!/bin/bash
# Tests the management of process IDs by the shared dummy backend. 

N_SUPPORTED_PROCESSES=10
CNT=0

declare -a PID

TEST_RESULT=0

# Start the supported number of processes in parallel
while [ $CNT -lt $N_SUPPORTED_PROCESSES ]; do 

    ../bin/testSharedDummyBackendExt --run_test=SharedDummyBackendTestSuite/testReadWrite KEEP_RUNNING & >/dev/null
    PID[$CNT]=$!

    echo "Started process #$CNT with PID: ${PID[$CNT]}"
    let CNT+=1
done

sleep .5 
#set -x

# All of those processes should be running now
CNT=0
while [ $CNT -lt $N_SUPPORTED_PROCESSES ]; do
   kill -0 ${PID[$CNT]}
   if [ $? -ne 0 ]; then
       echo "Testing PID Management of SharedDummyBackend failed. The required number of tests could not be created."
       TEST_RESULT=$(( $TEST_RESULT + 1 ))
   fi
   let CNT+=1
done


# Attempt to start another process, this should fail
../bin/testSharedDummyBackendExt --run_test=SharedDummyBackendTestSuite/testReadWrite KEEP_RUNNING & >/dev/null
PID_SURPLUS=$!

sleep .5 

#if [ $PID_SURPLUS -ne ${PID[$(( $N_SUPPORTED_PROCESSES - 1 ))]}  ]; then
if ps -p $PID_SURPLUS >/dev/null
then
    echo "Testint PID Management of SharedDummyBackend failed. Starting a supernumerous process has not been catched."
    #read -n 1 -p "Hit me"
    kill -s SIGINT $PID_SURPLUS
    TEST_RESULT=$(( $TEST_RESULT + 2 ))
fi


# Hard termination of one process, so that it does not clean up for itself.
# This tests, if the other processes correct this when they exit.
kill ${PID[0]}
CNT=1
while [ $CNT -lt $N_SUPPORTED_PROCESSES ]; do
    kill -s SIGINT ${PID[$CNT]}
    let CNT+=1
done

#TODO we need another process creating a mess, testReadWrite just echoes...
# Test, if memory has been cleaned up
../bin/testSharedDummyBackendExt --run_test=SharedDummyBackendTestSuite/testVerifyCleanup
CLEANUP_RESULT=$?

if [ CLEANUP_RESULT -ne 0 ]; then
    echo "Testing PID Management of SharedDummyBackend failed. Processes did not clean up when exiting."
    TEST_RESULT=$(( $TEST_RESULT + 4))
fi
exit $TEST_RESULT

