/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/discovery/discovery.hpp"
#include "codec/cbor/cbor.hpp"
#include "codec/cbor/streams_annotation.hpp"

namespace fc::markets::discovery {
  using libp2p::multi::Multiaddress;
  using libp2p::peer::PeerId;

  /**
   * @struct Everything needed to make a deal with a miner
   */
  struct PeerInfoCborable {
    std::vector<uint8_t> peer_id;
    std::vector<std::vector<uint8_t>> addresses;

    bool operator==(const PeerInfoCborable &rhs) const {
      return addresses == rhs.addresses && peer_id == rhs.peer_id;
    }

    static PeerInfoCborable fromRetrievalPeer(const PeerInfo &other) {
      std::vector<std::vector<uint8_t>> addresses;
      for (const auto &address : other.addresses) {
        addresses.push_back(address.getBytesAddress());
      }
      return PeerInfoCborable{.peer_id = other.id.toVector(),
                              .addresses = addresses};
    }

    outcome::result<PeerInfo> toRetrievalPeer() const {
      std::vector<Multiaddress> multiaddresses;
      for (const auto &address : addresses) {
        OUTCOME_TRY(multiaddress, Multiaddress::create(address));
        multiaddresses.push_back(multiaddress);
      }

      OUTCOME_TRY(peer_id, PeerId::fromBytes(peer_id));
      return PeerInfo{.id = peer_id, .addresses = multiaddresses};
    }
  };

  CBOR_TUPLE(PeerInfoCborable, peer_id, addresses);

  Discovery::Discovery(std::shared_ptr<Datastore> datastore)
      : datastore_{std::move(datastore)} {}

  outcome::result<void> Discovery::addPeer(const CID &cid,
                                           const PeerInfo &peer) {
    OUTCOME_TRY(cid_bytes, cid.toBytes());
    Buffer cid_key{cid_bytes};
    PeerInfoCborable peer_to_add = PeerInfoCborable::fromRetrievalPeer(peer);
    std::vector<PeerInfoCborable> peers_to_store;

    if (datastore_->contains(cid_key)) {
      OUTCOME_TRY(stored_peers_cbored, datastore_->get(cid_key));
      OUTCOME_TRY(peers,
                  codec::cbor::decode<std::vector<PeerInfoCborable>>(
                      stored_peers_cbored));
      // if already present
      if (std::find(peers.begin(), peers.end(), peer_to_add) != peers.end()) {
        return outcome::success();
      }
      peers_to_store = std::move(peers);
    }
    peers_to_store.push_back(peer_to_add);
    OUTCOME_TRY(peers_cbored, codec::cbor::encode(peers_to_store));
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
    OUTCOME_TRY(
        peers,
        codec::cbor::decode<std::vector<PeerInfoCborable>>(peers_cbored));
    std::vector<PeerInfo> result;
    for (const auto &peer_cborable : peers) {
      OUTCOME_TRY(peer, peer_cborable.toRetrievalPeer());
      result.push_back(peer);
    }
    return std::move(result);
  }

}  // namespace fc::markets::discovery
