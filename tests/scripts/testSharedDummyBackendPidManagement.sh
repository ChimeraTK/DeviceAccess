#!/bin/bash
# Tests the management of process IDs by the shared dummy backend. 

# acquire lock on shareddummyTest.dmap for the entire duration of the test (see also end of this file)
(

    flock 9 || exit 1

    N_SUPPORTED_PROCESSES=10
    CNT=0

    declare -a PID
    declare -a BGPIDS

    TEST_RESULT=0

    # Avoid processes continuing after termination 
    # of this script
    cleanup() {
        for pid in ${BGPIDS[@]}; do
            kill -9 $pid
        done
        # Call this test again, so leaked memory should be cleaned. 
        ./testSharedDummyBackendExt --run_test=SharedDummyBackendTestSuite/testVerifyCleanup
    }
    trap "cleanup" SIGINT SIGTERM EXIT


    check_process_termination() {
        PID=$1
        MAX_EVAL_CNT=$2
        TIMEOUT_CNT=0

        while [ $TIMEOUT_CNT -lt $MAX_EVAL_CNT ]; do
            sleep .2
            if ps -p $PID > /dev/null ; then
                let TIMEOUT_CNT+=1
                if [ $TIMEOUT_CNT -eq $MAX_EVAL_CNT ]; then
                    echo "    Failed to remove process $PID created for testing."
                    echo "    Further results may be corrupted!"
                fi
            else 
                break
            fi
        done     
    }


    ### Test capturing of supernumerous processes
    printf "\n##### Test capturing of starting supernumerous processes #####\n"
    # Start the supported number of processes in parallel
    while [ $CNT -lt $N_SUPPORTED_PROCESSES ]; do 

        ./testSharedDummyBackendExt --run_test=SharedDummyBackendTestSuite/testReadWrite -- KEEP_RUNNING & >/dev/null
        PID[$CNT]=$!
        BGPIDS+=("$!")

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
    #FIXME Boost 1.60 introduces a change in the command line interface, this call
    #      will issue an warning.
    ./testSharedDummyBackendExt --run_test=SharedDummyBackendTestSuite/testReadWrite -- KEEP_RUNNING & >/dev/null
    PID_SURPLUS=$!
    BGPIDS+=("$!")


    MAX_EVAL_CNT=20
    TIMEOUT_CNT=0
    while [ $TIMEOUT_CNT -lt $MAX_EVAL_CNT  ]; do 
        if ps -p $PID_SURPLUS >/dev/null ; # Process still exists
        then
            sleep .5
        else
            echo "    Supernumerous process was successfully terminated."
            break
        fi
        let TIMEOUT_CNT+=1

        if [ $TIMEOUT_CNT -eq $MAX_EVAL_CNT ]; then
            echo "Testing PID Management of SharedDummyBackend failed. Starting a supernumerous process has not been caught."
            ps -p $PID_SURPLUS ; # Show child status before killing it
            kill -s SIGINT $PID_SURPLUS
            TEST_RESULT=$(( $TEST_RESULT + 2 ))
        fi
    done


    # Kill all of the started processes and verify their termination
    CNT=0
    while [ $CNT -lt $N_SUPPORTED_PROCESSES ]; do
        kill -s SIGINT ${PID[$CNT]}
        check_process_termination ${PID[$CNT]} 20
        let CNT+=1
    done

    sleep .5



    ###### Test removal of processes, which were terminated but still have an entry in the PID list. #####
    printf "\n#####Test removal of dead processes from the PID list on construction.\n"

    # Start process writing some values, kill it
    ./testSharedDummyBackendExt --run_test=SharedDummyBackendTestSuite/testMakeMess &
    PID_MAKEMESS=$!
    sleep 1
    kill -9 $PID_MAKEMESS
    check_process_termination $PID_MAKEMESS 5

    # Test, if memory has been cleaned up. This should happen, when this test constructs the SharedMemoryManager
    ./testSharedDummyBackendExt --run_test=SharedDummyBackendTestSuite/testVerifyCleanup
    CLEANUP_RESULT=$?

    if [[ $CLEANUP_RESULT -ne 0 ]]; then
        echo "Testing PID Management of SharedDummyBackend failed. Shared memory has not been cleaned up properly."
        TEST_RESULT=$(( $TEST_RESULT + 4))
    fi

    sleep .5


    # Start processes, kill one of them. The still running one should clean up on exit
    printf "\n#####Test removal of dead processes from the PID list on deconstruction.\n"

    #FIXME Interface change in Boost 1.60, see above
    ./testSharedDummyBackendExt --run_test=SharedDummyBackendTestSuite/testReadWrite -- KEEP_RUNNING &
    PID_STILL_RUNNING_PROCESS=$!
    BGPIDS+=("$!")

    ./testSharedDummyBackendExt --run_test=SharedDummyBackendTestSuite/testMakeMess &
    sleep .2
    PID_MAKEMESS=$!
    kill -9 $PID_MAKEMESS
    check_process_termination $PID_MAKEMESS 5

    kill -s SIGINT $PID_STILL_RUNNING_PROCESS

    check_process_termination $PID_STILL_RUNNING_PROCESS 20

    # Test if memory has been deleted.
    ./testSharedDummyBackendExt --run_test=SharedDummyBackendTestSuite/testVerifyMemoryDeleted
    CLEANUP_RESULT=$?

    if [[ $CLEANUP_RESULT -ne 0  ]]; then
        echo "Testing PID management if SharedDummyBackend failed. Shared memory has not been cleaned up properly."
	        TEST_RESULT=$(( $TEST_RESULT + 8 ))
    fi

    exit $TEST_RESULT

# this is the end of the lock section
) 9<shareddummyTest.dmap
