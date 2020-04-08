# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(car
    car.cpp
    )
target_link_libraries(car
    cbor
    p2p::p2p_uvarint
    )
