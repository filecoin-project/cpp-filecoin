#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(filecoin_cid
    cid.cpp
    json_codec.cpp
    )

target_link_libraries(filecoin_cid
    Boost::boost
    filecoin_blake2
    filecoin_cbor
    p2p::p2p_cid
    )

add_library(filecoin_comm_cid
    comm_cid.cpp
    comm_cid_errors.cpp
    )

target_link_libraries(filecoin_comm_cid
    filecoin_cid
    filecoin_outcome
    )
