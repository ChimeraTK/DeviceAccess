#! /bin/bash

#
# This shell script calls the ParserUtilities test suite with the cwd as
# argument. The passed argument is used to validate the return value of
# getCurrentWorkingDirectory
../bin/testParserUtilities `pwd`
TEST_RESULT=$? 

exit $TEST_RESULT