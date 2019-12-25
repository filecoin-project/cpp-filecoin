#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(actor_test
    actor_test.cpp
    )
target_link_libraries(actor_test
    actor
    cbor
    hexutil
    )

addtest(init_actor_test
    init_actor_test.cpp
    )
target_link_libraries(init_actor_test
    actor
    hamt
    hexutil
    ipfs_datastore_in_memory
    )
