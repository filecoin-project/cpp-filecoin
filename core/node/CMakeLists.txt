#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(node
    blocksync.cpp
    hello.cpp
    peermgr.cpp
    pubsub.cpp
    )
target_link_libraries(node
    cbor_stream
    )

add_executable(node_main
    main.cpp
    )
target_link_libraries(node_main
    Boost::program_options
    clock
    node
    rpc
    )
