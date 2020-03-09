#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

filecoin_add_library(filecoin_piece
        piece.cpp
        piece_error.cpp
        )

target_link_libraries(filecoin_piece
        filecoin_cid
        filecoin_outcome
        )
