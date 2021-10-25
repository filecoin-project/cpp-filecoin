/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/discovery/impl/discovery_impl.hpp"
#include "codec/cbor/cbor_codec.hpp"

namespace fc::markets::discovery {

  DiscoveryImpl::DiscoveryImpl(std::shared_ptr<Datastore> datastore)
      : datastore_{std::move(datastore)} {}

  outcome::result<void> DiscoveryImpl::addPeer(const CID &cid,
                                               const RetrievalPeer &peer) {
    OUTCOME_TRY(cid_key, cid.toBytes());
    std::vector<RetrievalPeer> peers;

    if (datastore_->contains(cid_key)) {
      OUTCOME_TRY(stored_peers_cbored, datastore_->get(cid_key));
      OUTCOME_TRYA(
          peers,
          codec::cbor::decode<std::vector<RetrievalPeer>>(stored_peers_cbored));
      // if already present
      if (std::find(peers.begin(), peers.end(), peer) != peers.end()) {
        return outcome::success();
      }
    }
    peers.push_back(peer);
    OUTCOME_TRY(peers_cbored, codec::cbor::encode(peers));
    OUTCOME_TRY(datastore_->put(cid_key, std::move(peers_cbored)));
    return outcome::success();
  }

  outcome::result<std::vector<RetrievalPeer>> DiscoveryImpl::getPeers(
      const CID &cid) const {
    OUTCOME_TRY(cid_key, cid.toBytes());
    if (!datastore_->contains(cid_key)) {
      return std::vector<RetrievalPeer>{};
    }
    OUTCOME_TRY(peers_cbored, datastore_->get(cid_key));
    OUTCOME_TRY(peers,
                codec::cbor::decode<std::vector<RetrievalPeer>>(peers_cbored));
    return std::move(peers);
  }

}  // namespace fc::markets::discovery
