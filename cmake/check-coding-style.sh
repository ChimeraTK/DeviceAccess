#!/bin/bash

ERRFILE=`mktemp`
export ERRFILE
echo 0 > "${ERRFILE}"

# This is necessary to get the two paths to match up, since we get a full path from cmake
mypath=$(pwd)
mypath=$(realpath $mypath)
exclude_path=${1:-justignoreme}
exclude_pattern="$exclude_path/*"

# check clang-format formatting
if which clang-format-14 > /dev/null; then
  find $mypath \( -name \*.cc -o -name \*.cpp -o -name \*.h \) -not -path "$exclude_pattern" -exec clang-format-14 --output-replacements-xml \{\} \; | grep "^<replacement " > /dev/null
  if [ $? -ne 1 ]; then
    echo 1 > "${ERRFILE}"
    echo "Code formatting incorrect!"
  fi
else
  echo 77 > "${ERRFILE}"
  echo "WARNING: clang-format-14 not found, code formatting not checked!"
fi

# check copyright/licence file header comment
checkCopyrightComment() {
  SPDX_OK=1
  if [[ "`head -n1 $1`" != '// SPDX-FileCopyrightText: '* ]]; then
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
find $mypath \( -name \*.cc -o -name \*.cpp -o -name \*.h \) -not -path "$exclude_pattern" -exec bash -c 'checkCopyrightComment {}' \;

# check all header files for "#pragma once" in 3rd line
checkPramgaOnce() {
  if [ "`head -n3 $1 | tail -n1`" != '#pragma once' ]; then
    echo 1 > "${ERRFILE}"
    echo "Header $1 has no pragma once in 3rd line!"
  fi
}
export -f checkPramgaOnce
find $mypath -name \*.h -not -path "$exclude_pattern" -exec bash -c 'checkPramgaOnce {}' \;

ERROR=`cat "${ERRFILE}"`
rm -f "${ERRFILE}"

exit ${ERROR}
