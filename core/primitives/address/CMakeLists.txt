# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(address
    address.cpp
    address_codec.cpp
    ../chain_epoch/chain_epoch_codec.cpp)
target_link_libraries(address
    Boost::boost
    blake2
    blob
    cbor
    outcome
    )
