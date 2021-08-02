#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(light_actor_reader_test
    light_actor_reader.cpp
    )
target_link_libraries(light_actor_reader_test
    cbor
    miner_types
    actor_state
    ipfs_datastore_in_memory
    )
