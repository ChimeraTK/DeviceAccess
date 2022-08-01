#!/bin/bash

ERRFILE=`mktemp`
export ERRFILE
echo 0 > "${ERRFILE}"

# check clang-format formatting
if which clang-format-14 > /dev/null; then
  find \( -name *.cc -o -name *.cpp -o -name *.h \) -exec clang-format-14 --output-replacements-xml \{\} \; | grep "^<replacement " > /dev/null
  if [ $? -ne 1 ]; then
    echo 1 > "${ERRFILE}"
    echo "Code formatting incorrect!"
  fi
else
  echo "WARNING: clang-format-14 not found, code formatting not checked!"
fi

# check copyright/licence file header comment
checkCopyrightComment() {
  SPDX_OK=1
  if [[ "`head -n1 $1`" != '// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK'* ]]; then
    SPDX_OK=0
  fi
  if [[ "`head -n2 $1 | tail -n1`" != '// SPDX-License-Identifier: '* ]]; then
    SPDX_OK=0
  fi
  if [ $SPDX_OK -eq 0 ]; then
    echo 1 > "${ERRFILE}"
    echo "File $1 has no or an incorrect SPDX comment."
  fi
}
export -f checkCopyrightComment
find \( -name *.cc -o -name *.cpp -o -name *.h \) -exec bash -c 'checkCopyrightComment {}' \;

# check all header files for "#pragma once" in 3rd line
checkPramgaOnce() {
  if [ "`head -n3 $1 | tail -n1`" != '#pragma once' ]; then
    echo 1 > "${ERRFILE}"
    echo "Header $1 has no pragma once in 3rd line!"
  fi
}
export -f checkPramgaOnce
find -name *.h -exec bash -c 'checkPramgaOnce {}' \;

ERROR=`cat "${ERRFILE}"`
rm -f "${ERRFILE}"

exit ${ERROR}

