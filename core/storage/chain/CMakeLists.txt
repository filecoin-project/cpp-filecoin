# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(chain_store
    chain_store.cpp
    )
target_link_libraries(chain_store
    datastore_key
    ipfs_blockservice
    )
