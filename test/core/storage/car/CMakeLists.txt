#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(car_test
    car_test.cpp
    )
target_link_libraries(car_test
    block
    car
    ipfs_datastore_in_memory
    memory_indexed_car
    unixfs
    )

addtest(cids_index_test
    cids_index_test.cpp
    )
target_link_libraries(cids_index_test
    base_fs_test
    cids_index
    cids_ipld
    )

addtest(compacter_test
    compacter_test.cpp
    )
target_link_libraries(compacter_test
    base_fs_test
    compacter
    in_memory_storage
    )
