#! /bin/bash

currentWorkingDirectory=`pwd`
# run the test now
../bin/testParserUtilities `pwd`
TEST_RESULT=$? 

exit $TEST_RESULT