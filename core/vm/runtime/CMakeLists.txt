#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(circulating
    circulating.cpp
    )
target_link_libraries(circulating
    market_actor_state
    reward_actor_state
    storage_power_actor_state
    )

add_library(runtime
    runtime.cpp
    impl/env.cpp
    impl/runtime_impl.cpp
    impl/runtime_error.cpp
    impl/tipset_randomness.cpp
    )
target_link_libraries(runtime
    actor
    blake2
    cgo_actors
    circulating
    dvm
    fvm
    interpreter
    ipfs_datastore_error
    keystore
    message
    prometheus
    proofs
    tipset
    signature
    toolchain
    version
    )
