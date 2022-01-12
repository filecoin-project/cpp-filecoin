#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(api
    get_node.cpp
    make.cpp
    node_api_v1_wrapper.cpp
    )
target_link_libraries(api
    address
    block_producer
    cid
    const
    interpreter
    logger
    message
    miner_types
    msg_waiter
    node_version
    state_tree
    mpool
    sync
    jwt
    wallet
    )
