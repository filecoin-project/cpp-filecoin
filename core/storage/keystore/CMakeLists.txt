# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(keystore
    keystore.cpp
    keystore_error.cpp
    impl/filesystem/filesystem_keystore.cpp
    impl/in_memory/in_memory_keystore.cpp
    )
target_link_libraries(keystore
    address
    bls_provider
    filestore
    outcome
    signature
    )
