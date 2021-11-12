#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(deal_info_manager)

add_library(market_types
    v0/pending_proposals.cpp
    v3/pending_proposals.cpp
    bind_types.cpp
    )
target_link_libraries(market_types
    actor
    address
    const
    outcome
    )
