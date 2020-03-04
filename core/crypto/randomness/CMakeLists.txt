# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(randomness_provider
    impl/randomness_provider_impl.cpp
    )
target_link_libraries(randomness_provider
    Boost::boost
    blob
    buffer
    p2p::p2p_sha
    )

add_library(chain_randomness_provider
    impl/chain_randomness_provider_impl.cpp
    )
target_link_libraries(chain_randomness_provider
    tickets
    Boost::boost
    blob
    buffer
    p2p::p2p_sha
    )
