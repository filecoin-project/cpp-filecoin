/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "data_transfer/impl/graphsync/graphsync_manager.hpp"

#include <gtest/gtest.h>
#include <mock/libp2p/host/host_mock.hpp>
#include "testutil/literals.hpp"
#include "testutil/mocks/storage/ipfs/graphsync/graphsync_mock.hpp"
#include "testutil/outcome.hpp"
#include "testutil/peer_id.hpp"

namespace fc::data_transfer::graphsync {

  using libp2p::HostMock;
  using storage::ipfs::graphsync::GraphsyncMock;

  class GraphsyncManagerTest : public ::testing::Test {
   public:
    std::shared_ptr<HostMock> host = std::make_shared<HostMock>();
    PeerInfo peer_info{.id = generatePeerId(1), .addresses = {}};
    std::shared_ptr<GraphsyncMock> graphsync =
        std::make_shared<GraphsyncMock>();
  };

  /**
   * @given Graphsync manager with channel id created
   * @when try create the same channel
   * @then error returned
   */
  TEST_F(GraphsyncManagerTest, CreateChannelTwice) {
    EXPECT_CALL(*host, getPeerInfo())
        .WillOnce(::testing::Return(peer_info));
    GraphSyncManager manager{host, graphsync};

    TransferId transfer_id = 1;
    CID base_cid = "010001020005"_cid;
    auto selector = std::make_shared<Selector>();
    std::vector<uint8_t> voucher{};
    PeerInfo initiator{.id = generatePeerId(2), .addresses = {}};
    PeerInfo sender_peer{.id = generatePeerId(3), .addresses = {}};
    PeerInfo receiver_peer{.id = generatePeerId(4), .addresses = {}};

    EXPECT_OUTCOME_TRUE(channel_id,
                        manager.createChannel(transfer_id,
                                              base_cid,
                                              selector,
                                              voucher,
                                              initiator,
                                              sender_peer,
                                              receiver_peer));
    auto channel = manager.getChannelByIdAndSender(channel_id, sender_peer);
    EXPECT_TRUE(channel.has_value());

    EXPECT_OUTCOME_ERROR(GraphsyncManagerError::kStateAlreadyExists,
                         manager.createChannel(transfer_id,
                                               base_cid,
                                               selector,
                                               voucher,
                                               initiator,
                                               sender_peer,
                                               receiver_peer));
  }

}  // namespace fc::data_transfer::graphsync
