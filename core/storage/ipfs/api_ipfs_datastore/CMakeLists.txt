#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(api_ipfs_datastore
    api_ipfs_datastore.cpp
    api_ipfs_datastore_error.cpp
    )
target_link_libraries(api_ipfs_datastore
    outcome
    )
