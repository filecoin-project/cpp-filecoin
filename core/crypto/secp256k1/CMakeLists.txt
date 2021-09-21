# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(secp256k1_provider
    impl/secp256k1_provider_impl.cpp
    impl/secp256k1_sha256_provider_impl.cpp
    impl/secp256k1_error.cpp
    )
target_link_libraries(secp256k1_provider
    filecoin_sha
    libsecp256k1
    outcome
    )
