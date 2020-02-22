#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(message_pool)
add_subdirectory(production)

add_library(filecoin_block_validator
    impl/block_validator_impl.cpp
    )
target_link_libraries(filecoin_block_validator
    filecoin_block
    )

add_library(filecoin_weight_calculator
    impl/weight_calculator_impl.cpp
    )
target_link_libraries(filecoin_weight_calculator
    filecoin_tipset
    )
