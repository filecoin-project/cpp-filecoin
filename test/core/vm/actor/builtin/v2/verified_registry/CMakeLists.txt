#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(verified_registry_actor_v2_test
    verified_registry_actor_test.cpp
    )
target_link_libraries(verified_registry_actor_v2_test
    ipfs_datastore_in_memory
    verified_registry_actor_v2
    )
