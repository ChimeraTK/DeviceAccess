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
if which clang-format-19 > /dev/null; then
  find $mypath \( -name \*.cc -o -name \*.cpp -o -name \*.h \) -not -path "$exclude_pattern" -exec clang-format-19 --output-replacements-xml \{\} \; | grep "^<replacement " > /dev/null
  if [ $? -ne 1 ]; then
    echo 1 > "${ERRFILE}"
    echo "Code formatting incorrect!"
  fi
else
  echo 77 > "${ERRFILE}"
  echo "WARNING: clang-format-19 not found, code formatting not checked!"
fi

# check copyright/licence file header comment
checkCopyrightComment() {
  SPDX_OK=1
  # First line must be SPDX-FileCopyrightText
  if [[ "`head -n1 $1`" != '// SPDX-FileCopyrightText: '* ]]; then
    SPDX_OK=0
  fi
  # Find the first non-FileCopyrightText line and check if it's License-Identifier
  local found_license=0
  while IFS= read -r line; do
    if [[ "$line" == '// SPDX-FileCopyrightText: '* ]]; then
      continue
    elif [[ "$line" == '// SPDX-License-Identifier: '* ]]; then
      found_license=1
      break
    else
      # First non-FileCopyrightText line is not License-Identifier
      break
    fi
  done < "$1"
  if [ $found_license -eq 0 ]; then
    SPDX_OK=0
  fi
  if [ $SPDX_OK -eq 0 ]; then
    echo 1 > "${ERRFILE}"
    echo "File $1 has no or an incorrect SPDX comment."
  fi
}
export -f checkCopyrightComment
find $mypath \( -name \*.cc -o -name \*.cpp -o -name \*.h \) -not -path "$exclude_pattern" -exec bash -c 'checkCopyrightComment {}' \;

# check all header files for "#pragma once" immediately after SPDX-License-Identifier
checkPramgaOnce() {
  # Valid structure: SPDX-FileCopyrightText lines, then SPDX-License-Identifier, then #pragma once
  local state="copyright"
  local line_num=0
  while IFS= read -r line; do
    ((line_num++))
    case "$state" in
      copyright)
        if [[ "$line" == '// SPDX-FileCopyrightText: '* ]]; then
          continue
        elif [[ "$line" == '// SPDX-License-Identifier: '* ]]; then
          state="license_seen"
        else
          echo 1 > "${ERRFILE}"
          echo "Header $1 line $line_num: expected SPDX-FileCopyrightText or SPDX-License-Identifier, found '$line'!"
          return
        fi
        ;;
      license_seen)
        if [ "$line" != '#pragma once' ]; then
          echo 1 > "${ERRFILE}"
          echo "Header $1 line $line_num: expected #pragma once after SPDX-License-Identifier, found '$line'!"
        fi
        return
        ;;
    esac
  done < "$1"
  echo 1 > "${ERRFILE}"
  echo "Header $1 has incomplete SPDX header!"
}
export -f checkPramgaOnce
find $mypath -name \*.h -not -path "$exclude_pattern" -exec bash -c 'checkPramgaOnce {}' \;

ERROR=`cat "${ERRFILE}"`
rm -f "${ERRFILE}"
exit ${ERROR}
