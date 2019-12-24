#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(actor
    actor.cpp
    ../runtime/runtime.hpp)
target_link_libraries(actor
    p2p::p2p_cid
    p2p::p2p_sha
    )
