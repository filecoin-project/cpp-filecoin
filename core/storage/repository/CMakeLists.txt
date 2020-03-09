# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

filecoin_add_library(filecoin_repository
    impl/repository.cpp
    impl/repository_error.cpp
    )
target_link_libraries(filecoin_repository
    filecoin_outcome
    )

filecoin_add_library(filecoin_filesystem_repository
    impl/filesystem_repository.cpp
    )
target_link_libraries(filecoin_filesystem_repository
    Boost::filesystem
    filecoin_config
    filecoin_fslock
    filecoin_ipfs_datastore_leveldb
    filecoin_keystore
    filecoin_outcome
    filecoin_repository
    )

filecoin_add_library(filecoin_in_memory_repository
    impl/in_memory_repository.cpp
    )
target_link_libraries(filecoin_in_memory_repository
    filecoin_repository
    )
