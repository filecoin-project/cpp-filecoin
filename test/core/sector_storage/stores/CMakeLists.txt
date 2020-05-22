#
# Copyright Soramitsu Co., Ltd.All Rights Reserved.
# SPDX - License - Identifier : Apache - 2.0
#

addtest(sector_index_test
        index_test.cpp)

target_link_libraries(sector_index_test
        base_fs_test
        sector_index
        )
