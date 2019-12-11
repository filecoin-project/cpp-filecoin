# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(keystore_test
    filesystem_keystore_test.cpp
    in_memory_keystore_test.cpp
    )
target_link_libraries(keystore_test
    address
    base_fs_test
    keystore
    )
