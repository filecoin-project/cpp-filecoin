#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(market_actor_test
    test.cpp
    )
target_link_libraries(market_actor_test
    ipfs_datastore_in_memory
    market_actor
    state_tree
    )
