# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(genesis
    genesis.cpp
    )
target_link_libraries(genesis
    cbor
    p2p::p2p_uvarint
    )
