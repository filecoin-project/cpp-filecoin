#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(state_tree_test
    state_tree_test.cpp
    )
target_link_libraries(state_tree_test
    hexutil
    ipfs_datastore_in_memory
    state_tree
    )
