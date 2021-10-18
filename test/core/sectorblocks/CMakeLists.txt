#
# Copyright Soramitsu Co., Ltd.All Rights Reserved.
# SPDX - License - Identifier : Apache - 2.0
#

addtest(sectorblocks_test
        sectorblocks_test.cpp)

target_link_libraries(sectorblocks_test
        base_fs_test
        sectorblocks
        in_memory_storage
        )
