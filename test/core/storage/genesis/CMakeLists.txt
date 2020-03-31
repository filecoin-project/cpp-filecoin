#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(genesis_test
    genesis_test.cpp
    )
target_link_libraries(genesis_test
    genesis
    ipfs_datastore_in_memory
    )
target_include_directories(genesis_test PUBLIC
  "${CMAKE_CURRENT_BINARY_DIR}"
  )

configure_file(genesis_test.hpp.in genesis_test.hpp @ONLY)
