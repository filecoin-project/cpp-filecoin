# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(ticket
    ticket.cpp
    ticket_codec.cpp
    epost_ticket_codec.cpp
    )
target_link_libraries(ticket
    blob
    buffer
    p2p::p2p_sha
    )
