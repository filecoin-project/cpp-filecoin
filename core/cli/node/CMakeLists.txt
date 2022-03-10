#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_executable(fuhon-node-cli
    main.cpp
    )
target_link_libraries(fuhon-node-cli
    cli
    rpc
    car
    file
    memory_indexed_car
    unixfs
    api_ipfs_datastore
    )
set_target_properties(fuhon-node-cli PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
