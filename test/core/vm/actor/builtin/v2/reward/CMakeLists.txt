#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(reward_actor_v2_test
    reward_actor_v2_test.cpp
    )
target_link_libraries(reward_actor_v2_test
    reward_actor_v2
    ipfs_datastore_in_memory
    )
