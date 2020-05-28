#
# Copyright Soramitsu Co., Ltd.All Rights Reserved.
# SPDX - License - Identifier : Apache - 2.0
#

addtest(sector_storage_test
        sector_storage_test.cpp)

target_link_libraries( sector_storage_test
       Boost::filesystem
       sector_storage
       base_fs_test
       proof_param_provider
       )

add_subdirectory(stores)
