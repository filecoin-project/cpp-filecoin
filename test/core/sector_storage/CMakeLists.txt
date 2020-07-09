#
# Copyright Soramitsu Co., Ltd.All Rights Reserved.
# SPDX - License - Identifier : Apache - 2.0
#

addtest(local_worker_test
        local_worker_test.cpp)

target_link_libraries(local_worker_test
        Boost::filesystem
        worker
        base_fs_test
        proof_param_provider
        )

add_subdirectory(stores)
