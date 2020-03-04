#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(message_pool)
add_subdirectory(production)

add_library(block_validator
    impl/block_validator_impl.cpp
    )
target_link_libraries(block_validator
    block
    )

add_library(weight_calculator
    impl/weight_calculator_impl.cpp
    )
target_link_libraries(weight_calculator
    tipset
    )
