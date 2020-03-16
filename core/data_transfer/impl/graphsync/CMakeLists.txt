#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(data_transfer_graphsync
    graphsync_manager.cpp
    graphsync_receiver.cpp
    data_transfer_extension.cpp
    )
target_link_libraries(data_transfer_graphsync
    libp2p_data_transfer_network
    )
