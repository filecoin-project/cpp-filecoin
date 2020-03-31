#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(payment_channel_actor_test
    payment_channel_actor_test.cpp
    )
target_link_libraries(payment_channel_actor_test
    ipfs_datastore_in_memory
    payment_channel_actor
    state_tree
    )
