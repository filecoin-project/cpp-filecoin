#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(filecoin_state_tree_test
    state_tree_test.cpp
    )
target_link_libraries(filecoin_state_tree_test
    filecoin_hexutil
    filecoin_ipfs_datastore_in_memory
    filecoin_state_tree
    )
