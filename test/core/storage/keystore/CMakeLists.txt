# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(filecoin_keystore_test
    filesystem_keystore_test.cpp
    in_memory_keystore_test.cpp
    )
target_link_libraries(filecoin_keystore_test
    filecoin_address
    filecoin_base_fs_test
    filecoin_keystore
    )
