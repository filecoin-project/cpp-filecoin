#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(cids_index
    cids_index.cpp
    )
target_link_libraries(cids_index
    cid
    cids_ipld
    car
    file
    ipfs_datastore_error
    )

add_executable(cids_index_main
    main.cpp
    )
target_link_libraries(cids_index_main
    cids_index
    )
