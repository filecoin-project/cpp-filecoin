#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(filecoin_config
    profile_config.cpp
    )
target_link_libraries(filecoin_config
    Boost::boost
    const
    market_types
    miner_types
    payment_channel_types
    storage_power_types
    verified_registry_types
    )
