#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(payment_channel_actor
    payment_channel_actor.cpp
    payment_channel_actor_state.cpp
    )
target_link_libraries(payment_channel_actor
    outcome
    )
