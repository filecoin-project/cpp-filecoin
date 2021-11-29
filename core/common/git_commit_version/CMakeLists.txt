#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(git_commit_version
    git_commit_version.cpp
    )

# Get the git repo data
set(GIT_REPO_PRETTY_VERSION "version info unavailable")
if (EXISTS "${PROJECT_SOURCE_DIR}/.git")
  if (NOT GIT_FOUND)
    find_package(Git QUIET)
  endif ()

  if (GIT_EXECUTABLE)
    # Get pretty version
    execute_process(COMMAND
        "${GIT_EXECUTABLE}"
        describe --tags --always
        WORKING_DIRECTORY
        "${PROJECT_SOURCE_DIR}"
        RESULT_VARIABLE
        res
        OUTPUT_VARIABLE
        GIT_REPO_PRETTY_VERSION
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    if (NOT res EQUAL 0)
      message(STATUS "Error running `git describe': ${res}")
    endif ()
    message(STATUS "Git repo pretty version: ${GIT_REPO_PRETTY_VERSION}")
  else ()
    message(STATUS "Git executable not found!")
  endif ()
endif ()

target_compile_definitions(git_commit_version
    PRIVATE
    -DGIT_REPO_PRETTY_VERSION="${GIT_REPO_PRETTY_VERSION}"
    )
