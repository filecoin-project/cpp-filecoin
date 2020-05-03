#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(storage_market_provider
    provider.cpp
    provider_fsm_transitions.cpp
    )
target_link_libraries(storage_market_provider
    Boost::boost
    fsm
    outcome
    piece
    )
