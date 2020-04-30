#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(unixfs_test
    unixfs_test.cpp
    )
target_link_libraries(unixfs_test
    unixfs
    ipfs_datastore_in_memory
    )
