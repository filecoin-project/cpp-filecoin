#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(test_vectors_test
    test.cpp
    fixed_randomness.cpp
    replaying_randomness.cpp
    )
target_link_libraries(test_vectors_test
    Boost::iostreams
    car
    interpreter
    ipfs_datastore_in_memory
    json
    proof_param_provider
    state_tree
    runtime
    )

