#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(drand_http
    http.cpp
    )
target_link_libraries(drand_http
    blob
    buffer
    outcome
    json
    )

add_library(drand_beacon
    beaconizer.cpp
    )
target_link_libraries(drand_beacon
    bls_provider
    drand_http
    p2p::p2p_byteutil
    )
