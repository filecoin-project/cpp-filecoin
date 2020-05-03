#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(host_network_context
    network_context_impl.cpp
    )
target_link_libraries(host_network_context
    p2p::p2p_multiselect
    p2p::p2p_mplex
    p2p::p2p_yamux
    p2p::p2p_upgrader
    p2p::p2p_tcp
    p2p::p2p_router
    p2p::p2p_transport_manager
    p2p::p2p_connection_manager
    p2p::p2p_listener_manager
    p2p::p2p_dialer
    p2p::p2p_network
    p2p::p2p_peer_repository
    p2p::p2p_inmem_address_repository
    p2p::p2p_inmem_key_repository
    p2p::p2p_inmem_protocol_repository
    p2p::p2p_basic_host
    Boost::boost
    )
