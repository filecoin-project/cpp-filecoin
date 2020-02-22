# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

filecoin_add_library(filecoin_vrf_provider
    impl/vrf_provider_impl.cpp
    vrf_hash_encoder.cpp
    vrf_types.cpp
    )
target_link_libraries(filecoin_vrf_provider
    filecoin_address
    filecoin_blob
    filecoin_buffer
    p2p::p2p_sha
    )
