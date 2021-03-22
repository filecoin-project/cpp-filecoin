#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(market_types
    policy.cpp
    )
target_link_libraries(market_types
    const
    )
