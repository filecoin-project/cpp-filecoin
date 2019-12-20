# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(repository_filesystem
    impl/repository.cpp
    impl/repository_error.cpp
    impl/filesystem_repository.cpp
    )
target_link_libraries(repository_filesystem
    Boost::filesystem
    config
    fslock
    outcome
    keystore
    ipfs_datastore_leveldb
    )
