#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(graphsync)

add_library(libp2p_data_transfer_network
    libp2p_data_transfer_network.cpp
    )
target_link_libraries(libp2p_data_transfer_network
    p2p::p2p
    )
