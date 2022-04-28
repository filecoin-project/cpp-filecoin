#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_executable(diff
    main.cpp
    )
target_link_libraries(diff
    car
    cids_index
    ipfs_datastore_error
    rle_plus_codec
    tipset
    )
set_target_properties(diff PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    )
