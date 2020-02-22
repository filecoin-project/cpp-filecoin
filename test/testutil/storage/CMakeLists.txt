# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(filecoin_base_fs_test
    base_fs_test.hpp
    base_fs_test.cpp
    )
target_link_libraries(filecoin_base_fs_test
    Boost::filesystem
    Boost::boost
    filecoin_logger
    )

add_library(filecoin_base_leveldb_test
    base_leveldb_test.hpp
    base_leveldb_test.cpp
    )
target_link_libraries(filecoin_base_leveldb_test
    filecoin_base_fs_test
    Boost::filesystem
    Boost::boost
    filecoin_logger
    filecoin_leveldb
    )

add_library(filecoin_std_list_adapter INTERFACE)

target_link_libraries(filecoin_std_list_adapter INTERFACE
    filecoin_outcome
    )
