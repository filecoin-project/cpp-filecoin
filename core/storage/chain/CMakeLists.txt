# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(chain_store
    impl/chain_store_impl.cpp
    )
target_link_libraries(chain_store
    chain_randomness_provider
    datastore_key
    ipfs_blockservice
    logger
    )

add_library(datastore_key
    datastore_key.cpp
    )
target_link_libraries(datastore_key
    Boost::boost
    Boost::filesystem
    )

add_library(chain_data_store
    impl/chain_data_store_impl.cpp
    )
target_link_libraries(chain_data_store
    datastore_key
    )
