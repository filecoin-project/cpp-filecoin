# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(filecoin_chain_data_store_test
    chain_data_store_test.cpp
    )
target_link_libraries(filecoin_chain_data_store_test
    filecoin_chain_data_store
    filecoin_ipfs_datastore_in_memory
    )
