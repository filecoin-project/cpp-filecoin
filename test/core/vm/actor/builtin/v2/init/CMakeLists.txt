#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(init_actor_test_v2
    init_actor_test.cpp
    )
target_link_libraries(init_actor_test_v2
    init_actor_v2
    ipfs_datastore_in_memory
    )
