# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(filecoin_randomness_provider
    impl/randomness_provider_impl.cpp
    )
target_link_libraries(filecoin_randomness_provider
    Boost::boost
    filecoin_blob
    filecoin_buffer
    p2p::p2p_sha
    )

add_library(filecoin_chain_randomness_provider
    impl/chain_randomness_provider_impl.cpp
    )
target_link_libraries(filecoin_chain_randomness_provider
    Boost::boost
    filecoin_blob
    filecoin_buffer
    p2p::p2p_sha
    )
