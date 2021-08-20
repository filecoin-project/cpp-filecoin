#
# Copyright Soramitsu Co., Ltd.All Rights Reserved.
# SPDX - License - Identifier : Apache - 2.0
#

add_subdirectory(stores)
add_subdirectory(zerocomm)

addtest(selector_test
        allocate_selector_test.cpp
        existing_selector_test.cpp
        task_selector_test.cpp
        )

target_link_libraries(selector_test
        selector
        )

addtest(local_worker_test
        local_worker_test.cpp)

target_link_libraries(local_worker_test
        Boost::filesystem
        worker
        base_fs_test
        proof_param_provider
        )

addtest(scheduler_test
        scheduler_test.cpp)

target_link_libraries(scheduler_test
        scheduler
        base_fs_test
        in_memory_storage
        )

addtest(manager_test
        manager_test.cpp)

target_link_libraries(manager_test
        manager
        base_fs_test
        Boost::filesystem
        )
