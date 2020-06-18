#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_executable(drand_example
    drand_example.cpp
    )
target_link_libraries(drand_example
    drand_client
    )

add_executable(drand_three_rounds
    three_rounds.cpp
    )
target_link_libraries(drand_three_rounds
    drand_client
    )


add_executable(drand_beacon_verifier
    beacon_verifier.cpp
    )
target_link_libraries(drand_beacon_verifier
    drand_client
    p2p::p2p_byteutil
    p2p::p2p_sha
    bls_provider
    hexutil
    )
