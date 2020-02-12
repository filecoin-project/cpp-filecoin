#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(array_test
    array_test.cpp
    )
target_link_libraries(array_test
    amt
    array
    ipfs_datastore_in_memory
    )


addtest(multimap_test
    multimap_test.cpp
    )
target_link_libraries(multimap_test
    hamt
    multimap
    ipfs_datastore_in_memory
    )
