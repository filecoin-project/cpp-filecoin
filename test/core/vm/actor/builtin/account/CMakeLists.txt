#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(filecoin_account_actor_test
    account_actor_test.cpp
    )
target_link_libraries(filecoin_account_actor_test
    filecoin_account_actor
    filecoin_state_tree
    filecoin_ipfs_datastore_in_memory
    )
