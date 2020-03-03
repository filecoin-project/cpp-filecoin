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

add_library(sync_manager
    impl/sync_manager_impl.cpp
    )
target_link_libraries(sync_manager
    block
    Boost::boost
    syncer_state
    tipset
    )

add_library(syncer_state
    syncer_state.cpp
    )
target_link_libraries(syncer_state
    clock
    tipset
    )
