#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(car_test
    car_test.cpp
    )
target_link_libraries(car_test
    car
    ipfs_datastore_in_memory
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

addtest(genesis_test
    genesis_test.cpp
    )
target_link_libraries(genesis_test
    block
    car
    ipfs_datastore_in_memory
    state_tree
    )
