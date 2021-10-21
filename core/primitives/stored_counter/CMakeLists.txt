#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(stored_counter
        stored_counter.cpp
        )
target_link_libraries(stored_counter
        outcome
        p2p::p2p_uvarint
        )
