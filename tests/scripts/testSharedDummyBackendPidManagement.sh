#!/bin/bash
# Tests the management of process IDs by the shared dummy backend. 

N_SUPPORTED_PROCESSES=10
CNT=0

declare -a PID

TEST_RESULT=0

##### Test capturing of starting supernumerous processes #####
# Start the supported number of processes in parallel
while [ $CNT -lt $N_SUPPORTED_PROCESSES ]; do 

    ../bin/testSharedDummyBackendExt --run_test=SharedDummyBackendTestSuite/testReadWrite KEEP_RUNNING & >/dev/null
    PID[$CNT]=$!

    echo "Started process #$CNT with PID: ${PID[$CNT]}"
    let CNT+=1
done

sleep .5 

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

if ps -p $PID_SURPLUS >/dev/null
then
    echo "Testint PID Management of SharedDummyBackend failed. Starting a supernumerous process has not been catched."
    kill -s SIGINT $PID_SURPLUS
    TEST_RESULT=$(( $TEST_RESULT + 2 ))
fi

CNT=0
while [ $CNT -lt $N_SUPPORTED_PROCESSES ]; do
    kill -s SIGINT ${PID[$CNT]}
    let CNT+=1
done



###### Test removal of processes, which were terminated but still have an entry in the PID list. #####

# Start process writing some values, kill it
../bin/testSharedDummyBackendExt --run_test=SharedDummyBackendTestSuite/testMakeMess &
sleep .2
kill $!

# Test, if memory has been cleaned up. This should happen, when this test constructs the SharedMemoryManager
../bin/testSharedDummyBackendExt --run_test=SharedDummyBackendTestSuite/testVerifyCleanup
CLEANUP_RESULT=$?

if [[ CLEANUP_RESULT -ne 0 ]]; then
    echo "Testing PID Management of SharedDummyBackend failed. Shared memory has not been cleaned up properly."
    TEST_RESULT=$(( $TEST_RESULT + 4))
fi




# Start processes, kill one of them. The still running one should clean up on exit
../bin/testSharedDummyBackendExt --run_test=SharedDummyBackendTestSuite/testReadWrite KEEP_RUNNING &
../bin/testSharedDummyBackendExt --run_test=SharedDummyBackendTestSuite/testMakeMess &
sleep .2
kill $!



exit $TEST_RESULT

