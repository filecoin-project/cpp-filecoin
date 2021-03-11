#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(multisig_types
    transaction.cpp
    )
target_link_libraries(multisig_types
    actor
    address
    outcome
    )
