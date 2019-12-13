#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(address
    address/address_builder.cpp
    address/address_verifier.cpp
    address.cpp
    address_codec.cpp
    )

target_link_libraries(address
    Boost::boost
    blake2
    blob
    leb128
    outcome
    )
