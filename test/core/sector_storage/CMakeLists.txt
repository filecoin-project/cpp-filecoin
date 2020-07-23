#
# Copyright Soramitsu Co., Ltd.All Rights Reserved.
# SPDX - License - Identifier : Apache - 2.0
#

add_subdirectory(stores)

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
        )

addtest(manager_test
        manager_test.cpp)

target_link_libraries(manager_test
        manager
        )
