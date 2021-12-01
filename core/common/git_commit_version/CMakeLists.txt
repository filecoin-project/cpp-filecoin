#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_custom_target(git_commit_version_generated
    COMMAND ./git_commit_version.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    BYPRODUCTS ${CMAKE_CURRENT_SOURCE_DIR}/git_commit_version.cpp
    )
add_library(git_commit_version
    git_commit_version.cpp
    )
add_dependencies(git_commit_version
    git_commit_version_generated
    )
