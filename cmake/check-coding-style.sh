#!/bin/bash

ERRFILE=$(mktemp)
export ERRFILE
echo 0 > "${ERRFILE}"

# This is necessary to get the two paths to match up, since we get a full path from cmake
mypath=$(pwd)
mypath=$(realpath "$mypath")
exclude_path=${1:-justignoreme}
exclude_pattern="$exclude_path/*"

# check clang-format formatting
if which clang-format-19 > /dev/null; then
  find "$mypath" \( -name \*.cc -o -name \*.cpp -o -name \*.h \) -not -path "$exclude_pattern" -exec clang-format-19 --output-replacements-xml \{\} \; | grep "^<replacement " > /dev/null
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
  # First line must be SPDX-FileCopyrightText
  if [[ "$(head -n1 "$1")" != '// SPDX-FileCopyrightText: '* ]]; then
    echo 1 > "${ERRFILE}"
    echo "File $1 does not start with SPDX-FileCopyrightText line."
  fi
  # Find the first non-FileCopyrightText line and check if it's License-Identifier
  local found_license=0
  local found_pragma=0
  local found_copyright=0
  local check_pragma=0
  local line_num=0
  while IFS= read -r line; do
    ((line_num++))
    if [[ $check_pragma -eq 0 && "$line" == '// SPDX-FileCopyrightText: '* ]]; then
      found_copyright=1
      continue
    elif [[ $check_pragma -eq 0 && "$line" == '// SPDX-License-Identifier: '* ]]; then
      found_license=1
      if [[ "$1" != *.h ]]; then
        break        
      else
        check_pragma=1
        continue
      fi
    elif [ $check_pragma -eq 1 ]; then
      if [ "$line" == '#pragma once' ]; then
        found_pragma=1
      fi
      break
    else
      # First non-FileCopyrightText line is not License-Identifier
      echo "Unexpected line when testing SPDX comments and pragma statement. File $1 line $line_num: '$line'"
      break
    fi
  done < "$1"
  if [ $found_license -eq 0 ]; then
    echo 1 > "${ERRFILE}"
    echo "File $1 has no or an incorrect SPDX-License-Identifier comment."
  fi
  if [ $check_pragma -eq 1 ] && [ $found_pragma -eq 0 ]; then
    echo 1 > "${ERRFILE}"
    echo "Header $1 has wrong/is missing '#pragma once' after SPDX-License-Identifier comment."
  fi
  if [ $found_copyright -eq 0 ]; then
    echo 1 > "${ERRFILE}"
    echo "File $1 has no or an incorrect SPDX-FileCopyrightText comment."
  fi
}
export -f checkCopyrightComment
find "$mypath" \( -name \*.cc -o -name \*.cpp -o -name \*.h \) -not -path "$exclude_pattern" -exec bash -c 'checkCopyrightComment "$1"' _ {} \;

ERROR=`cat "${ERRFILE}"`
rm -f "${ERRFILE}"
exit ${ERROR}
