#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(provider)
add_subdirectory(network)

add_library(fuhon_stored_ask
    stored_ask.cpp
    )
target_link_libraries(fuhon_stored_ask
    p2p::p2p
    )
