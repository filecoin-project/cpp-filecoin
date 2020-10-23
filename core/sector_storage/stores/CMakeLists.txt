#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(sector_index
        impl/index_impl.cpp
        impl/index_lock.cpp
        )

target_link_libraries(sector_index
        sector_file
        outcome
        uri_parser
        Boost::filesystem
        logger
        )

add_library(storage_error
        impl/storage_error.cpp
        )
target_link_libraries(storage_error
        outcome
        )

add_library(store
        impl/local_store.cpp
        impl/store_error.cpp
        impl/remote_store.cpp
        )

target_link_libraries(store
        buffer
        CURL::libcurl
        file
        logger
        rpc
        sector_index
        tarutil
        storage_error
        p2p::asio_scheduler
        )
