#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(account_actor_v3_test
    account_actor_test.cpp
    )
target_link_libraries(account_actor_v3_test
    account_actor_v3
    ipfs_datastore_in_memory
    )
