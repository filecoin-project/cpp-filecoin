# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(chain_store_test
    chain_store_test.cpp
    )
target_link_libraries(chain_store_test
    chain_data_store
    chain_randomness_provider
    chain_store
    block_validator
    ipfs_datastore_in_memory
    weight_calculator
    )
