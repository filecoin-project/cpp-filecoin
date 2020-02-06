#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(array_test
    array_test.cpp
    )
target_link_libraries(array_test
    array
    ipfs_datastore_in_memory
    )
