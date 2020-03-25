#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(block_validator
    impl/block_validator_impl.cpp
    impl/syntax_rules.cpp
    impl/consensus_rules.cpp
    )
target_link_libraries(block_validator
    bls_provider
    outcome
    buffer
    address
    clock
    interpreter
    tipset
    power_table
    )
