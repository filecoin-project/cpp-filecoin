#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(stores)
add_subdirectory(zerocomm)

add_library(fetch_handler
        impl/fetch_handler.cpp
        )

target_link_libraries(fetch_handler
        outcome
        store
        logger
        tarutil
        rpc
        Boost::boost
        )

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

add_library(remote_worker
        impl/remote_worker.cpp
        )

target_link_libraries(remote_worker
        worker
        rpc
        )

add_library(estimator
        impl/worker_estimator_impl.cpp
        )

target_link_libraries(estimator
        Boost::boost
        )

add_library(scheduler
        impl/scheduler_impl.cpp
        impl/new_scheduler_impl.cpp
        )

target_link_libraries(scheduler
        outcome
        resources
        logger
        worker
        estimator
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
        worker
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
        tarutil
        )
