#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(pieceio
    pieceio_impl.cpp
    pieceio_error.cpp
    )
target_link_libraries(pieceio
    car
    comm_cid
    piece
    piece_data
    proofs
    piece_storage
    )
