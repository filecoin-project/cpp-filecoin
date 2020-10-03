#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(test_vectors_test
    test.cpp
    )
target_link_libraries(test_vectors_test
    Boost::iostreams
    car
    file
    ipfs_datastore_in_memory
    json
    state_tree
    runtime
    )

