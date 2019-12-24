#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(hamt
    hamt.cpp
    )
target_link_libraries(hamt
    blake2
    blob
    murmur
    outcome
    p2p::p2p_cid
    )
