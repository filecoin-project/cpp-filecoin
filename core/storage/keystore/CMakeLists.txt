# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

filecoin_add_library(filecoin_keystore
    keystore.cpp
    keystore_error.cpp
    impl/filesystem/filesystem_keystore.cpp
    impl/in_memory/in_memory_keystore.cpp
    )
target_link_libraries(filecoin_keystore
    filecoin_address
    filecoin_bls_provider
    filecoin_filestore
    filecoin_outcome
    filecoin_signature
    )
