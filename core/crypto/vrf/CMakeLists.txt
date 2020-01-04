# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(vrf_provider
    impl/vrf_provider_impl.cpp
    vrfhash_provider.cpp
    vrf_types.cpp
    )
target_link_libraries(vrf_provider
    address
    blob
    buffer
    p2p::p2p_sha
    )
