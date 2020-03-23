# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(sync_target_bucket_test
    sync_target_bucket_test.cpp
    )
target_link_libraries(sync_target_bucket_test
    sync_target_bucket
    )

addtest(sync_bucket_set_test
    sync_bucket_set_test.cpp
    )
target_link_libraries(sync_bucket_set_test
    sync_bucket_set
    )

addtest(sync_manager_test
    sync_manager_test.cpp
    )
target_link_libraries(sync_manager_test
    sync_manager
    )
