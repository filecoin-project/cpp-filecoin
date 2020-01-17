#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(cid
    cid.cpp
    )
target_link_libraries(cid
    blake2
    p2p::p2p_cid
    )
