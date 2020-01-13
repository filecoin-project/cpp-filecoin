# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(ticket_utils
    ticket_utils.cpp
    )
target_link_libraries(ticket_utils
    p2p::p2p_random_generator
    blob
    buffer
    tickets
    )
