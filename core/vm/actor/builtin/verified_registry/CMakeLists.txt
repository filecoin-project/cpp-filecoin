#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(verified_registry
    actor.cpp
    )
target_link_libraries(verified_registry
    actor
    outcome
    )
