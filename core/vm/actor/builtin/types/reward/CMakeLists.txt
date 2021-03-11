#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(reward_types
    reward_actor_calculus.cpp
    )
target_link_libraries(reward_types
    actor
    )
