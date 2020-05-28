#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(stores)

add_library(sector_storage
        impl/sector_storage_impl.cpp
        impl/sector_storage_error.cpp
        )

target_link_libraries(sector_storage
        filecoin_ffi
        outcome
        piece
        sector
        piece_data
        sector_file
        proofs
        Boost::filesystem
        )
