#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(data_transfer_test
    message_test.cpp
    libp2p_data_transfer_network_test.cpp
    stream_message_sender_test.cpp
    graphsync/data_transfer_extension_test.cpp
    graphsync/graphsync_receiver_test.cpp
    graphsync/graphsync_manager_test.cpp
    )
target_link_libraries(data_transfer_test
    actor
    cbor
    data_transfer
    data_transfer_graphsync
    libp2p_data_transfer_network
    )

add_executable(data_transfer_example
    data_transfer_example.cpp
    )
target_link_libraries(data_transfer_example
    data_transfer
    data_transfer_graphsync
    libp2p_data_transfer_network
    )

add_executable(data_transfer_example_2
    data_transfer_example_2.cpp
    )
target_link_libraries(data_transfer_example_2
    data_transfer
    data_transfer_graphsync
    libp2p_data_transfer_network
    )
