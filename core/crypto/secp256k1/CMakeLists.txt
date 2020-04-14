# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(secp256k1_provider
    secp256k1_provider_impl.cpp
    )
target_link_libraries(secp256k1_provider
    libsecp256k1
    p2p::p2p_secp256k1_provider
    )
