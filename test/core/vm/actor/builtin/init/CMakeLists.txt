#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(filecoin_init_actor_test
    init_actor_test.cpp
    )
target_link_libraries(filecoin_init_actor_test
    filecoin_init_actor
    filecoin_ipfs_datastore_in_memory
    filecoin_state_tree
    )
