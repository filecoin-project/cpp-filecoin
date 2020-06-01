#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(sector_index
        impl/index_impl.cpp
        )

target_link_libraries(sector_index
        sector_file
        outcome
        uri_parser
        Boost::filesystem
        )

add_library(local_store
        impl/local_store.cpp
        )

target_link_libraries(local_store
        sector_index
        )
