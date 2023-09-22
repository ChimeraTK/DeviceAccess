#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
# SPDX-FileCopyRightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile


def make_absolute(file: str, path: str):
    if os.path.isabs(file):
        return file
    return os.path.normpath(os.path.join(path, file))


def main():
    parser = argparse.ArgumentParser(description="Runs clang-tidy over a compilation database and applies the fixes")
    parser.add_argument("compile_db", metavar='PATH')
    parser.add_argument("--tidy", help="Path to clang-tidy", default="clang-tidy-14")
    parser.add_argument("--apply-tool", help='Path to clang-apply-replacements', default="clang-apply-replacements-14")
    parser.add_argument("--git", help="Path to git", default="git")
    parser.add_argument("--source", help="Path to top-level source folder", default=".")
    parser.add_argument("--exclude", help="Regex of files to exclude", default=None)

    args = parser.parse_args()

    apply_version = subprocess.run([args.apply_tool, "--version"], capture_output=True,
                                   encoding="latin1").stdout.strip()

    version_regex = re.compile(r"ersion\s*(\d+)\.(\d+)\.(\d+)")
    match = version_regex.search(apply_version)
    has_ignore_insert_conflict = int(match.group(1)) > 14

    files = None

    try:
      database = json.load(open(args.compile_db))
      files = set([make_absolute(entry['file'], entry['directory']) for entry in database])
    except FileNotFoundError:
        print(f"Failed to open {args.compile_db}: Not found")
    except json.decoder.JSONDecodeError:
        print(f"Failed to open {args.compile_db}: Not valid json")


    if not files:
        sys.exit(1)

    tmpdir = tempfile.mkdtemp()
    source_folder = os.path.abspath(args.source)

    exclude_re = re.compile(args.exclude) if args.exclude else None

    for file in files:
        if exclude_re and exclude_re.search(file):
            continue
        tidy = [args.tidy, "-header-filter=.*", "-export-fixes"]
        (handle, name) = tempfile.mkstemp(suffix='.yaml', dir=tmpdir)
        os.close(handle)
        tidy.append(name)
        tidy.append("-p=" + os.path.dirname(os.path.normpath(args.compile_db)))
        tidy.append(file)
        try:
            print(f"Linting {file}...")
            subprocess.run(tidy, check=True)
        except subprocess.CalledProcessError:
            print("Linting failed. Usually that means the previously applied fix has introduced a compiler error. Aborting.")
            print("Most likely it converted a conversion operator to explicit")
            sys.exit(1)

        fixer = [args.apply_tool, "--format", "--style=file", "--remove-change-desc-files"]
        if has_ignore_insert_conflict:
            fixer.append('--ignore-insert-conflict')
        fixer.append(tmpdir)
        if subprocess.call(fixer) == 0:
            git_update = [args.git, 'add', '-u']
            subprocess.call(git_update)
            git_commit = [args.git, 'commit', '-m', f'clang-tidy: {os.path.relpath(file, start=source_folder)}']
            subprocess.call(git_commit)
        else:
            print("Error applying fixes, not committing")
    shutil.rmtree(tmpdir)


if __name__ == "__main__":
    main()
