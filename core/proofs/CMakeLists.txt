#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(proofs
        proofs.cpp
        proofs_error.cpp
        )

target_link_libraries(proofs
        filecoin_ffi
        outcome
        blob
        logger
        comm_cid
        piece
        address
        sector
        )
