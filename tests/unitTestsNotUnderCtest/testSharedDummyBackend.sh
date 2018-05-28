#!/bin/bash
# Script to call the robustness/performance test processes for the SharedDummyBackend
# TODO The whole test should be cycled several times to test creation and
#      destruction of the shared memory

N_CYCLES=$1

CNT=0

while [ $CNT -lt $N_CYCLES ]; do 

    ../bin/testSharedDummyBackend2ndApp KEEP_RUNNING &
    PID=$!

    ../bin/testSharedDummyBackendPerformance 1000

    kill -s SIGINT $PID
    
    let CNT+=1
done

