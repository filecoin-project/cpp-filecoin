#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(init_actor_v2
    init_actor.cpp
    )
target_link_libraries(init_actor_v2
    actor
    outcome
    hamt
    )
