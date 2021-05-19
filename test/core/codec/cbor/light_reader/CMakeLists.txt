#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(light_actor_reader
    light_actor_reader.cpp
    )
target_link_libraries(light_actor_reader
    cbor
    storage_power_actor_v0
    ipfs_datastore_in_memory
    )
