# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(indexdb_test
    indexdb_test.cpp
    )
target_link_libraries(indexdb_test
    indexdb
    Boost::filesystem
    )
