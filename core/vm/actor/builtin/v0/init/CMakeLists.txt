#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(init_actor_v0
    init_actor.cpp
    )
target_link_libraries(init_actor_v0
    init_actor_state
    actor
    outcome
    hamt
    )
