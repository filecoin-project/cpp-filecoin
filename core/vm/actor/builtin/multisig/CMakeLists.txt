#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(filecoin_multisig_actor
    multisig_actor.cpp
    )
target_link_libraries(filecoin_multisig_actor
    filecoin_actor
    filecoin_outcome
    )
