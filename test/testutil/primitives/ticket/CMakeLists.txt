# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(filecoin_ticket_generator
    ticket_generator.cpp
    )
target_link_libraries(filecoin_ticket_generator
    p2p::p2p_random_generator
    filecoin_blob
    filecoin_buffer
    filecoin_tickets
    )

add_library(filecoin_ticket_printer
    printer.cpp
    )
target_link_libraries(filecoin_ticket_printer
    Boost::boost
    filecoin_blob
    filecoin_buffer
    filecoin_tickets
    )
