# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(address
    address.cpp
    address_codec.cpp
    impl/address_verifier_impl.cpp
    )
target_link_libraries(address
    Boost::boost
    blake2
    blob
    bls_provider
    cbor
    outcome
    p2p::p2p_secp256k1_provider
    )
