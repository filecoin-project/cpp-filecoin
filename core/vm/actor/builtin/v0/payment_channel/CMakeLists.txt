#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(payment_channel_actor_v0
    payment_channel_actor.cpp
    payment_channel_actor_utils.cpp
    )
target_link_libraries(payment_channel_actor_v0
    payment_channel_actor_state
    actor
    array
    outcome
    signature
    )
