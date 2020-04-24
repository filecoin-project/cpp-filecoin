#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(piece
        impl/piece.cpp
        impl/piece_error.cpp
        )

target_link_libraries(piece
        cid
        outcome
        )

add_library(piece_data
        impl/piece_data.cpp
        )

target_link_libraries(piece_data
        Boost::filesystem
        )
