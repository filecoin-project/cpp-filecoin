/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "data_transfer/impl/graphsync/graphsync_manager.hpp"

#include <gtest/gtest.h>
#include <mock/libp2p/host/host_mock.hpp>
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/peer_id.hpp"

namespace fc::data_transfer::graphsync {

  using libp2p::HostMock;

  class GraphsyncManagerTest : public ::testing::Test {
   public:
    std::shared_ptr<HostMock> host = std::make_shared<HostMock>();
    PeerId peer_id = generatePeerId(1);

    GraphSyncManager manager{host, peer_id};
  };

  /**
   * @given Graphsync manager with channel id created
   * @when try create the same channel
   * @then error returned
   */
  TEST_F(GraphsyncManagerTest, CreateChannelTwice) {
    TransferId transfer_id = 1;
    CID base_cid = "010001020005"_cid;
    std::shared_ptr<IPLDNode> selector;
    std::vector<uint8_t> voucher{};
    PeerId initiator = generatePeerId(2);
    PeerId sender_peer = generatePeerId(3);
    PeerId receiver_peer = generatePeerId(4);

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

    EXPECT_OUTCOME_ERROR(GraphsyncManagerError::STATE_ALREADY_EXISTS,
                         manager.createChannel(transfer_id,
                                               base_cid,
                                               selector,
                                               voucher,
                                               initiator,
                                               sender_peer,
                                               receiver_peer));
  }

}  // namespace fc::data_transfer::graphsync
