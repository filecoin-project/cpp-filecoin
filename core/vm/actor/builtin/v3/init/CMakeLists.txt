#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(init_actor_v3
    init_actor.cpp
    init_actor_state.cpp
    init_actor_utils.cpp
    )
target_link_libraries(init_actor_v3
    actor
    outcome
    hamt
    )
