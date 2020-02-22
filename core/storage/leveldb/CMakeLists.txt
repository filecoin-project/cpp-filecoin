# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(filecoin_leveldb
    leveldb.cpp
    leveldb_batch.cpp
    leveldb_error.cpp
    leveldb_cursor.cpp
    )
target_link_libraries(filecoin_leveldb
    leveldb::leveldb
    filecoin_buffer
    filecoin_logger
    )
