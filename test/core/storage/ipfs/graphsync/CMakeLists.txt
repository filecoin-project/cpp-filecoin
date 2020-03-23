#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(graphsync_acceptance_test
    graphsync_acceptance_test.cpp
    graphsync_acceptance_common.cpp
    )
target_link_libraries(graphsync_acceptance_test
    graphsync
    ipfs_merkledag_service
    p2p::asio_scheduler
    )

addtest(graphsync_extension_test
    extensions_test.cpp
    )
target_link_libraries(graphsync_extension_test
    graphsync
    )
