#!/bin/bash

# Configuration value: maximum percentage a newly measured value is allowed exceed the stored reference
MAXPERCENT=20

if [ -z "$1" -o ! -f "$1" ]; then
  echo Usage: ./run_performance_test.sh /path/to/reference/file.txt
  exit 1
fi
REFERENCE="$1"

../bin/testAccessorPerformance 1000

# check if number of results in the new output file is equal to the stored reference
NVARS=`wc -l performance_test.txt | sed -e 's/ .*$//'`
if [ "$NVARS" != "`wc -l $REFERENCE | sed -e 's/ .*$//'`" ]; then
  echo "The number of results in the reference and the new test result differ. Cannot compare."
  exit 1
fi

# compare all results with the stored reference
FAILUREFOUND=0
for (( i = 1 ; i <= NVARS ; i++ )); do

  # extract the lines from the files
  NEWLINE=`head -n $i performance_test.txt | tail -n 1`
  REFLINE=`head -n $i "$REFERENCE" | tail -n 1`
  
  # check if the result name is the same
  NEWVAR=`echo $NEWLINE | sed -e 's/=.*$//'`
  REFVAR=`echo $REFLINE | sed -e 's/=.*$//'`
  if [ "$NEWVAR" != "$REFVAR" ]; then
    echo "Line $i contains different result names: ${NEWVAR} is not ${REFVAR}!"
    exit 1
  fi
  
  # extract the values
  NEWVAL=`echo $NEWLINE | sed -e 's/^.*=//'`
  REFVAL=`echo $REFLINE | sed -e 's/^.*=//'`
  
  # compare values with tolerance
  THRESHOLD=$(( REFVAL * (MAXPERCENT+100) / 100 ))
  if [ $NEWVAL -gt $THRESHOLD ]; then
    FAILUREFOUND=1
    echo "Result $NEWVAR = $NEWVAL exceeds the stored reference $REFVAL by more than $MAXPERCENT percent!"
  fi

done

# print final result and set exit code
if [ "$FAILUREFOUND" != "0" ]; then
  echo "*** Performance problem found!"
  exit 1
fi
echo "No performance problem found."
exit 0
