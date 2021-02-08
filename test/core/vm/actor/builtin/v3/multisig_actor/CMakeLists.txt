#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(multisig_actor_test_v3
    multisig_actor_test.cpp
    )
target_link_libraries(multisig_actor_test_v3
    multisig_actor_v3
    ipfs_datastore_in_memory
    )
