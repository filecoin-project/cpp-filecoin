#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(reward_actor_v0_test
    reward_actor_v0_test.cpp
    reward_actor_calculus_v0_test.cpp
    )
target_link_libraries(reward_actor_v0_test
    reward_actor_v0
    ipfs_datastore_in_memory
    )
