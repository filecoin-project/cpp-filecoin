#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(storage_market_import_manager
    import_manager.cpp
    )
target_link_libraries(storage_market_import_manager
    Boost::boost
    car
    file
    outcome
    unixfs
    ipfs_datastore_in_memory
    )
