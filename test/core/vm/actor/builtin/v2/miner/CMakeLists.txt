#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(miner_actor_v2_test
    miner_actor_v2_test.cpp
    )
target_link_libraries(miner_actor_v2_test
    miner_actor_v2
    ipfs_datastore_in_memory
    )

