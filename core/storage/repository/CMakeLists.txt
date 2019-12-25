# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(repository
    impl/repository.cpp
    impl/repository_error.cpp
    )
target_link_libraries(repository
    outcome
    )

add_library(filesystem_repository
    impl/filesystem_repository.cpp
    )
target_link_libraries(filesystem_repository
    Boost::filesystem
    config
    fslock
    ipfs_datastore_leveldb
    keystore
    outcome
    repository
    )

add_library(in_memory_repository
    impl/in_memory_repository.cpp
    )
target_link_libraries(in_memory_repository
    repository
    )
