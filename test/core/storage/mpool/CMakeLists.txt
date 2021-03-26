#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(mpool_test
    message_pool_test.cpp
    )
target_link_libraries(mpool_test
    car
    in_memory_storage
    interpreter
    ipfs_datastore_in_memory
    mpool
    )
