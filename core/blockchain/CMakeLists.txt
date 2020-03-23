#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(block_validator)
add_subdirectory(message_pool)
add_subdirectory(production)

add_library(weight_calculator
    impl/weight_calculator_impl.cpp
    )
target_link_libraries(weight_calculator
    state_tree
    tipset
    )

add_library(sync_manager
    impl/sync_manager_impl.cpp
    )
target_link_libraries(sync_manager
    block
    Boost::boost
    cid
    syncer_state
    sync_bucket_set
    sync_target_bucket
    tipset
    )

add_library(syncer_state
    syncer_state.cpp
    )
target_link_libraries(syncer_state
    clock
    tipset
    )

add_library(sync_target_bucket
    impl/sync_target_bucket.cpp
    )
target_link_libraries(sync_target_bucket
    Boost::boost
    tipset
    )

add_library(sync_bucket_set
    impl/sync_bucket_set.cpp
    )
target_link_libraries(sync_bucket_set
    sync_target_bucket
    )
