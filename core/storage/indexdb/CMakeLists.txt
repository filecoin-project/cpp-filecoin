# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(indexdb
    indexdb_impl.cpp
    graph.cpp
    )
target_link_libraries(indexdb
    p2p::libp2p_sqlite
    buffer
    cid
    tipset
    logger
    )
