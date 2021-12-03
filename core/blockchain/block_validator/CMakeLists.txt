#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(block_validator
    validator.cpp
    )
target_link_libraries(block_validator
    bls_provider
    outcome
    address
    clock
    interpreter
    map_prefix
    tipset
    )
