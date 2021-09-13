#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(system_actor_v2
    system_actor.cpp
    )
target_link_libraries(system_actor_v2
    system_actor_state
    actor
    )
