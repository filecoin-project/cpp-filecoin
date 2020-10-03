#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(runtime
    impl/env.cpp
    impl/runtime_impl.cpp
    impl/runtime_error.cpp)
target_link_libraries(runtime
    actor
    blake2
    bls_provider
    cgo_actors
    keystore
    message
    proofs
    secp256k1_provider
    tipset
    )
