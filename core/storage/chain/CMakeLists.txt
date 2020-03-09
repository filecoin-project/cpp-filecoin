# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

filecoin_add_library(filecoin_chain_store
    impl/chain_store_impl.cpp
    )
target_link_libraries(filecoin_chain_store
    filecoin_chain_randomness_provider
    filecoin_datastore_key
    filecoin_ipfs_blockservice
    filecoin_logger
    )

filecoin_add_library(filecoin_datastore_key
    datastore_key.cpp
    )
target_link_libraries(filecoin_datastore_key
    Boost::boost
    Boost::filesystem
    )

filecoin_add_library(filecoin_chain_data_store
    impl/chain_data_store_impl.cpp
    )
target_link_libraries(filecoin_chain_data_store
    filecoin_datastore_key
    )
