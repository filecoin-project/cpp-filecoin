#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(zerocomm
    zerocomm.cpp
    )

target_link_libraries(zerocomm
    cid
    comm_cid
    piece
    )
