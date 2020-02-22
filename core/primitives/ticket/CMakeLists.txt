# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

filecoin_add_library(filecoin_tickets
    ticket.cpp
    epost_ticket.cpp
    ticket_codec.cpp
    epost_ticket_codec.cpp
    )
target_link_libraries(filecoin_tickets
    filecoin_blob
    filecoin_buffer
    filecoin_cbor
    p2p::p2p_sha
    filecoin_address
    )
