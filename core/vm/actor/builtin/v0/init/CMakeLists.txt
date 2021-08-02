#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(init_actor_v0
    init_actor.cpp
    init_actor_utils.cpp
    )
target_link_libraries(init_actor_v0
    actor
    outcome
    hamt
    )
