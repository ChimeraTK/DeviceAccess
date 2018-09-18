#!/bin/bash
# Script to call the robustness/performance test processes for the SharedDummyBackend

N_CYCLES=$1
N_RW_CYCLES=$2

CNT=0

while [ $CNT -lt $N_CYCLES ]; do 

    ./testSharedDummyBackendExt --run_test=SharedDummyBackendTestSuite/testReadWrite KEEP_RUNNING & >/dev/null
    PID=$!

    sleep .2

    ./testSharedDummyBackendExt --run_test=SharedDummyBackendTestSuite/testRobustnessMain $N_RW_CYCLES
    RET_MAIN=$?

    kill -s SIGINT $PID
    RET_2ND=$? 

    if [[ RET_MAIN -ne 0 ]] || [[ RET_2ND -ne 0 ]]; then
        printf "Test interrupted. Return values:\n    $RET_MAIN, $RET_2ND\n"
        break 
    fi

    let CNT+=1
done

