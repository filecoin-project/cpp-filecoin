#!/bin/bash -xe

## on master branch: analyzes all cpp files
## on other branches: analyzes cpp files changed in this branch, in comparison to master

function get_abs_path(){
    echo $(echo "$(cd "$(dirname "$1")"; pwd -P)/$(basename "$1")")
}

function get_files(){
    local head=$(git rev-parse --abbrev-ref HEAD)
    if [[ "${head}" = "master" ]]; then
        # EXPLAIN: all commited files
        git ls-files
    else
        # EXPLAIN: local unstaged changes
        git diff --name-only
        # EXPLAIN: local staged changes
        git diff --name-only --cached
        # TODO: check this works on CI (what if it already merged changes)
        # EXPLAIN: commited changes
        git diff --name-only HEAD..origin/master
    fi
}

BUILD_DIR=$(get_abs_path $1)
cd $(dirname $0)/..

# EXPLAIN: core/**/*.cpp files
FILES=$(get_files | grep '^core\/.*\.cpp$')

# LOCAL: replace CLANG_TIDY and RUN_CLANG_TIDY to downloaded version
CLANG_TIDY=$(which clang-tidy)
RUN_CLANG_TIDY=$(which run-clang-tidy.py)

# filter compile_commands.json
echo ${FILES} | python3 ./housekeeping/filter_compile_commands.py -p ${BUILD_DIR}

# exec run-clang-tidy.py
python3 ${RUN_CLANG_TIDY} -clang-tidy-binary=${CLANG_TIDY} -p ${BUILD_DIR}/filter_compile_commands
