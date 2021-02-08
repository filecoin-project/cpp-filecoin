#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(multisig_actor_v3
    impl/multisig_utils_impl_v3.cpp
    multisig_actor.cpp
    )
target_link_libraries(multisig_actor_v3
    actor
    outcome
    )
