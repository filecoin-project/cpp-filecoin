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

add_library(remote_index
        impl/remote_index_impl.cpp
        )

target_link_libraries(remote_index
        sector_index
        rpc
        )

add_library(store
        impl/local_store.cpp
        impl/storage_error.cpp
        impl/storage_impl.cpp
        impl/store_error.cpp
        impl/remote_store.cpp
        )

target_link_libraries(store
        CURL::libcurl
        file
        logger
        json
        sector_index
        tarutil
        )
