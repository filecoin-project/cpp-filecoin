#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(account_actor_test_v3
    account_actor_test.cpp
    )
target_link_libraries(account_actor_test_v3
    account_actor_v3
    state_tree
    ipfs_datastore_in_memory
    )
