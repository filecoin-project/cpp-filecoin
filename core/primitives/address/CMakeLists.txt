# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

filecoin_add_library(filecoin_address
    address.cpp
    address_codec.cpp
        ../piece/piece.hpp ../piece/piece.hpp)
target_link_libraries(filecoin_address
    Boost::boost
    filecoin_blake2
    filecoin_blob
    filecoin_cbor
    filecoin_outcome
    )
