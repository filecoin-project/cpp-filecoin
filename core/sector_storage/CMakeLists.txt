#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(stores)

add_library(worker
        impl/local_worker.cpp
        impl/worker_error.cpp
        )

target_link_libraries(worker
        outcome
        sector_file
        piece
        store
        proofs
        logger
        )

add_library(scheduler
        impl/scheduler_impl.cpp
        )

target_link_libraries(scheduler
        outcome
        resources
        logger
        worker
        Boost::thread
        )

add_library(selector
        impl/allocate_selector.cpp
        impl/existing_selector.cpp
        impl/task_selector.cpp
        )

target_link_libraries(selector
        outcome
        resources
        )

add_library(manager
        impl/manager_impl.cpp
        )

target_link_libraries(manager
        outcome
        scheduler
        selector
        store
        sector_index
        sector
        )
