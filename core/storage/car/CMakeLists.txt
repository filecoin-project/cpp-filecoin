# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(car
    car.cpp
    )
target_link_libraries(car
    file
    ipld_traverser
    p2p::p2p_uvarint
    )

add_subdirectory(cids_index)
