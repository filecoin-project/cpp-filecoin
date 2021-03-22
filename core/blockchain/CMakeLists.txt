#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(block_validator)
add_subdirectory(production)

add_library(weight_calculator
    impl/weight_calculator_impl.cpp
    )
target_link_libraries(weight_calculator
    state_tree
    tipset
    )

