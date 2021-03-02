#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(multisig_actor_v3
    multisig_actor.cpp
    multisig_actor_utils.cpp
    )
target_link_libraries(multisig_actor_v3
    actor
    multisig_types
    outcome
    )
