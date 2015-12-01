#! /bin/bash

../bin/RebotDummyServer -m ./mtcadummy.map&
sleep .1

# run the test now
../bin/testRebotDevice mskrebot ./dummies.dmap
TEST_RESULT=$? 

killall RebotDummyServer
exit $TEST_RESULT