# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(deal_info_manager)

addtest(deal_proposal_test
    deal_proposal_test.cpp
    )
target_link_libraries(deal_proposal_test
    market_types
    )
