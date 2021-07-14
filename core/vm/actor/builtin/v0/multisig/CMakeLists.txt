#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(multisig_actor_v0
    multisig_actor.cpp
    multisig_actor_utils.cpp
    )
target_link_libraries(multisig_actor_v0
    actor
    multisig_types
    outcome
    )
