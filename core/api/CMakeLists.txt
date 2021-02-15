#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(api
    make.cpp
    get_node.cpp
    )
target_link_libraries(api
    address
    block_producer
    cid
    const
    interpreter
    message
    msg_waiter
    state_tree
    todo_error
    )

add_library(rpc
    rpc/json_errors.cpp
    rpc/ws.cpp
    rpc/wsc.cpp
    )
target_link_libraries(rpc
    api
    json
    tipset
    )
