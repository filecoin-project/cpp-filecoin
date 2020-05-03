#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(host_security_context
    security_context_impl.cpp
    )
target_link_libraries(host_security_context
    p2p::p2p_crypto_key
    p2p::p2p_key_validator
    p2p::p2p_key_marshaller
    p2p::p2p_identity_manager
    p2p::p2p_secio
    )
