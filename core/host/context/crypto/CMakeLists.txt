#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(host_crypto_context
    crypto_context_impl.cpp
    )
target_link_libraries(host_crypto_context
    p2p::p2p_crypto_provider
    )
