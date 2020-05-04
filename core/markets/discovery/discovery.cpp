/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/discovery/discovery.hpp"
#include "codec/cbor/cbor.hpp"
#include "common/libp2p/peer/cbor_peer_info.hpp"

namespace fc::markets::discovery {

  Discovery::Discovery(std::shared_ptr<Datastore> datastore)
      : datastore_{std::move(datastore)} {}

  outcome::result<void> Discovery::addPeer(const CID &cid,
                                           const PeerInfo &peer) {
    OUTCOME_TRY(cid_bytes, cid.toBytes());
    Buffer cid_key{cid_bytes};
    std::vector<PeerInfo> peers;

    if (datastore_->contains(cid_key)) {
      OUTCOME_TRY(stored_peers_cbored, datastore_->get(cid_key));
      OUTCOME_TRYA(
          peers,
          codec::cbor::decode<std::vector<PeerInfo>>(stored_peers_cbored));
      // if already present
      if (std::find(peers.begin(), peers.end(), peer) != peers.end()) {
        return outcome::success();
      }
    }
    peers.push_back(peer);
    OUTCOME_TRY(peers_cbored, codec::cbor::encode(peers));
    OUTCOME_TRY(datastore_->put(cid_key, peers_cbored));
    return outcome::success();
  }

  outcome::result<std::vector<PeerInfo>> Discovery::getPeers(
      const CID &cid) const {
    OUTCOME_TRY(cid_bytes, cid.toBytes());
    Buffer cid_key{cid_bytes};
    if (!datastore_->contains(cid_key)) {
      return std::vector<PeerInfo>{};
    }
    OUTCOME_TRY(peers_cbored, datastore_->get(cid_key));
    OUTCOME_TRY(peers,
                codec::cbor::decode<std::vector<PeerInfo>>(peers_cbored));
    return std::move(peers);
  }

}  // namespace fc::markets::discovery
