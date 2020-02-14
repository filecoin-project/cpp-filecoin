#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(reward_actor
    reward_actor.cpp
    )
target_link_libraries(reward_actor
    Boost::boost
    actor
    address
    exit_code
    outcome
    )
