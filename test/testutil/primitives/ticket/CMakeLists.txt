# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(ticket_generator
    ticket_generator.cpp
    )
target_link_libraries(ticket_generator
    p2p::p2p_random_generator
    blob
    buffer
    tickets
    )

add_library(ticket_printer
    printer.cpp
    )
target_link_libraries(ticket_printer
    Boost::boost
    blob
    buffer
    tickets
    )
