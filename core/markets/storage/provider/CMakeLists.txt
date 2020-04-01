#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(storage_market_provider
    provider.cpp
    )
target_link_libraries(storage_market_provider
    Boost::boost
    outcome
    )
