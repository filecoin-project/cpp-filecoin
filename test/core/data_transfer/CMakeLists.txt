#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(data_transfer_test
    message_test.cpp
    libp2p_data_transfer_network_test.cpp
    )

target_link_libraries(data_transfer_test
    actor
    cbor
    data_transfer_graphsync
    libp2p_data_transfer_network
    )
