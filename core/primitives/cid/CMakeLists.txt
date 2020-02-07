#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(cid
    cid.cpp
    json_codec.cpp
    )
target_link_libraries(cid
    Boost::boost
    blake2
    p2p::p2p_cid
    )
