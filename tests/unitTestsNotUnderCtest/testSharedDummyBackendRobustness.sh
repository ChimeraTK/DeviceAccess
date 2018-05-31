    #!/bin/bash
# Script to call the robustness/performance test processes for the SharedDummyBackend
# TODO The whole test should be cycled several times to test creation and
#      destruction of the shared memory

N_CYCLES=$1
N_RW_CYCLES=$2

CNT=0

while [ $CNT -lt $N_CYCLES ]; do 

    ../bin/testSharedDummyBackendExt --run_test=SharedDummyBackendTestSuite/testReadWrite KEEP_RUNNING & >/dev/null
    PID=$!

    ../bin/testSharedDummyBackendExt --run_test=SharedDummyBackendTestSuite/testRobustnessMain $N_RW_CYCLES
    RET_MAIN=$?

    kill -s SIGINT $PID
    RET_2ND=$? 

    if [[ RET_MAIN -ne 0 ]] || [[ RET_2ND -ne 0 ]]; then
        printf "Test interrupted. Return values:\n    $RET_MAIN, $RET_2ND\n"
        break 
    fi

    let CNT+=1
done

