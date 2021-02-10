#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(runtime
    circulating.cpp
    impl/actor_context/actor_context_v0.cpp
    impl/actor_context/actor_context_v2.cpp
    impl/actor_context/actor_context_v3.cpp
    impl/actor_context/actor_context_creator.cpp
    impl/env.cpp
    impl/runtime_impl.cpp
    impl/runtime_error.cpp
    impl/tipset_randomness.cpp
    )
target_link_libraries(runtime
    actor
    blake2
    bls_provider
    cgo_actors
    dvm
    ipfs_datastore_error
    ipld_traverser
    keystore
    message
    proofs
    secp256k1_provider
    tipset
    signature
    )
