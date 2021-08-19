# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(base_fs_test
    base_fs_test.hpp
    base_fs_test.cpp
        ../../core/vm/actor/builtin/types/miner/monies_test_v2.cpp ../../core/vm/actor/builtin/types/miner/monies_test_v0.cpp ../../core/vm/actor/builtin/types/miner/monies_test_v3.cpp)
target_link_libraries(base_fs_test
    Boost::filesystem
    Boost::boost
    logger
    )

add_library(base_leveldb_test
    base_leveldb_test.hpp
    base_leveldb_test.cpp
    )
target_link_libraries(base_leveldb_test
    base_fs_test
    Boost::filesystem
    Boost::boost
    logger
    leveldb
    )
