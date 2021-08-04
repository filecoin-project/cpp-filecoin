#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(storage_power_actor_v2
    storage_power_actor_export.cpp
    storage_power_actor_utils.cpp
    )
target_link_libraries(storage_power_actor_v2
    storage_power_actor_state
    storage_power_actor_v0
    )
