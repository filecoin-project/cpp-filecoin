#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(runtime
    runtime.cpp
    circulating.cpp
    impl/env.cpp
    impl/runtime_impl.cpp
    impl/runtime_error.cpp
    impl/tipset_randomness.cpp
    )
target_link_libraries(runtime
    actor
    blake2
    cgo_actors
    dvm
    interpreter
    ipfs_datastore_error
    keystore
    message
    proofs
    tipset
    signature
    toolchain
    actor_state
    version
    )
