# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(filecoin_leveldb_fs_test
    leveldb_fs_test.cpp
    )
target_link_libraries(filecoin_leveldb_fs_test
    filecoin_leveldb
    filecoin_base_fs_test
    )

addtest(filecoin_leveldb_integration_test
    leveldb_integration_test.cpp
    )
target_link_libraries(filecoin_leveldb_integration_test
    filecoin_leveldb
    filecoin_base_leveldb_test
    Boost::filesystem
    )
