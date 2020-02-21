# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(chain_data_store_test
    chain_data_store_test.cpp
    )
target_link_libraries(chain_data_store_test
    chain_data_store
    ipfs_datastore_in_memory
    )
