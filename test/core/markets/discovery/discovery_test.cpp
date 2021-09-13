/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/discovery/impl/discovery_impl.hpp"

#include <gmock/gmock.h>
#include "storage/in_memory/in_memory_storage.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/peer_id.hpp"

namespace fc::markets::discovery {
  using libp2p::multi::Multiaddress;
  using libp2p::peer::PeerInfo;
  using primitives::address::Address;
  using storage::InMemoryStorage;

  class DiscoveryTest : public ::testing::Test {
   public:
    std::shared_ptr<Datastore> datastore = std::make_shared<InMemoryStorage>();
    DiscoveryImpl discovery{datastore};

    CID proposal_cid_1 = "010001020001"_cid;
    CID proposal_cid_2 = "010001020002"_cid;
    PeerId peer_id_1 = generatePeerId(1);
    PeerId peer_id_2 = generatePeerId(2);
    Address address_1{Address::makeFromId(1)};
    Address address_2{Address::makeFromId(2)};
    RetrievalPeer retrieval_peer_1{address_1, peer_id_1, boost::none};

    template <typename T>
    bool vectorHas(std::vector<T> vec, const T &element) const {
      return (std::find(vec.begin(), vec.end(), element) != vec.end());
    }
  };

  /**
   * @given empty datastore
   * @when try get peer by some cid
   * @then empty vector returned
   */
  TEST_F(DiscoveryTest, empty) {
    EXPECT_OUTCOME_TRUE(empty_peers, discovery.getPeers(proposal_cid_1));
    EXPECT_TRUE(empty_peers.empty());
  }

  /**
   * @given discovery with retrieval_peer_1
   * @when try add the same peer
   * @then success returned, state doesn't changed
   */
  TEST_F(DiscoveryTest, addTheSame) {
    EXPECT_OUTCOME_TRUE_1(discovery.addPeer(proposal_cid_1, retrieval_peer_1));

    // check initial state
    EXPECT_OUTCOME_TRUE(initial_peers, discovery.getPeers(proposal_cid_1));
    EXPECT_EQ(initial_peers.size(), 1);
    EXPECT_EQ(initial_peers.front(), retrieval_peer_1);

    EXPECT_OUTCOME_TRUE_1(discovery.addPeer(proposal_cid_1, retrieval_peer_1));
    // check state
    EXPECT_OUTCOME_TRUE(peers, discovery.getPeers(proposal_cid_1));
    EXPECT_EQ(peers.size(), 1);
    EXPECT_EQ(peers.front(), retrieval_peer_1);
  }

  /**
   * @given discovery with retrieval_peer_1
   * @when try add the same peer with different proposal cid
   * @then success returned, state has 2 peer with different proposals
   */
  TEST_F(DiscoveryTest, addProposal) {
    EXPECT_OUTCOME_TRUE_1(discovery.addPeer(proposal_cid_1, retrieval_peer_1));

    EXPECT_OUTCOME_TRUE(initial_peers, discovery.getPeers(proposal_cid_1));
    EXPECT_EQ(initial_peers.size(), 1);
    EXPECT_EQ(initial_peers.front(), retrieval_peer_1);

    EXPECT_OUTCOME_TRUE_1(discovery.addPeer(proposal_cid_2, retrieval_peer_1));
    EXPECT_OUTCOME_TRUE(peers, discovery.getPeers(proposal_cid_2));
    EXPECT_EQ(peers.size(), 1);
    EXPECT_EQ(peers.front(), retrieval_peer_1);
  }

  /**
   * @given discovery with retrieval_peer_1
   * @when try add different peer with the same proposal cid
   * @then success returned, state has all peers
   */
  TEST_F(DiscoveryTest, addPeers) {
    EXPECT_OUTCOME_TRUE_1(discovery.addPeer(proposal_cid_1, retrieval_peer_1));

    RetrievalPeer retrieval_peer_2{address_2, peer_id_2, proposal_cid_2};
    EXPECT_OUTCOME_TRUE_1(discovery.addPeer(proposal_cid_1, retrieval_peer_2));
    EXPECT_OUTCOME_TRUE(peers_2, discovery.getPeers(proposal_cid_1));
    EXPECT_EQ(peers_2.size(), 2);
    EXPECT_TRUE(vectorHas(peers_2, retrieval_peer_1));
    EXPECT_TRUE(vectorHas(peers_2, retrieval_peer_2));

    RetrievalPeer retrieval_peer_3{address_2, peer_id_1, boost::none};
    EXPECT_OUTCOME_TRUE_1(discovery.addPeer(proposal_cid_1, retrieval_peer_3));
    EXPECT_OUTCOME_TRUE(peers_3, discovery.getPeers(proposal_cid_1));
    EXPECT_EQ(peers_3.size(), 3);
    EXPECT_TRUE(vectorHas(peers_3, retrieval_peer_1));
    EXPECT_TRUE(vectorHas(peers_3, retrieval_peer_2));
    EXPECT_TRUE(vectorHas(peers_3, retrieval_peer_3));
  }

}  // namespace fc::markets::discovery
