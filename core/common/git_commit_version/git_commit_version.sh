#!/bin/sh
set -e

GIT_REPO_PRETTY_VERSION=$(git describe --tags --always --dirty) || GIT_REPO_PRETTY_VERSION="git-unknown"
FILE=$(sed -e "s/@GIT_REPO_PRETTY_VERSION@/$GIT_REPO_PRETTY_VERSION/g" git_commit_version.cpp.in)
OLD_FILE=$(cat git_commit_version.cpp) || OLD_FILE=""
HASH=$(echo "$FILE" | shasum)
OLD_HASH=$(echo "$OLD_FILE" | shasum)

# if [ "$HASH" != "$OLD_HASH" ]; then
#   echo "git_commit_version.sh: update"
#   echo "$FILE" > git_commit_version.cpp
# fi
