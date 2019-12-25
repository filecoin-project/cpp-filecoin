#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(actor
    actor.cpp
    init_actor.cpp
    )
target_link_libraries(actor
    address
    p2p::p2p_cid
    p2p::p2p_sha
    )
