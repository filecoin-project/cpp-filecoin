/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/discovery/discovery.hpp"
#include "codec/cbor/cbor.hpp"
#include "codec/cbor/streams_annotation.hpp"

namespace fc::markets::discovery {
  using libp2p::peer::PeerId;
  using storage::ipfs::IpfsDatastoreError;

  /**
   * @struct Everything needed to make a deal with a miner
   */
  struct RetrievalPeerCborable {
    Address address;
    std::vector<uint8_t> peer_id;

    bool operator==(const RetrievalPeerCborable &rhs) const {
      return address == rhs.address && peer_id == rhs.peer_id;
    }

    static RetrievalPeerCborable fromRetrievalPeer(const RetrievalPeer &other) {
      return RetrievalPeerCborable{.address = other.address,
                                   .peer_id = other.id.toVector()};
    }

    outcome::result<RetrievalPeer> toRetrievalPeer() const {
      OUTCOME_TRY(peer_id, PeerId::fromBytes(peer_id));
      return RetrievalPeer{.address = address, .id = peer_id};
    }
  };

  CBOR_TUPLE(RetrievalPeerCborable, address, peer_id);

  Discovery::Discovery(std::shared_ptr<IpfsDatastore> datastore)
      : datastore_{std::move(datastore)} {}

  outcome::result<void> Discovery::addPeer(const CID &cid,
                                           const RetrievalPeer &peer) {
    auto maybe_peers = datastore_->get(cid);
    if (maybe_peers.has_error()
        && maybe_peers.error() != IpfsDatastoreError::NOT_FOUND) {
      return maybe_peers.error();
    }

    RetrievalPeerCborable peer_to_add =
        RetrievalPeerCborable::fromRetrievalPeer(peer);
    std::vector<RetrievalPeerCborable> peers_to_store;
    if (maybe_peers.has_value()) {
      OUTCOME_TRY(peers,
                  codec::cbor::decode<std::vector<RetrievalPeerCborable>>(
                      maybe_peers.value()));
      // if already present
      if (std::find(peers.begin(), peers.end(), peer_to_add) != peers.end()) {
        return outcome::success();
      }
      peers_to_store = std::move(peers);
    }
    peers_to_store.push_back(peer_to_add);
    OUTCOME_TRY(peers_cbored, codec::cbor::encode(peers_to_store));
    OUTCOME_TRY(datastore_->set(cid, peers_cbored));
    return outcome::success();
  }

  outcome::result<std::vector<RetrievalPeer>> Discovery::getPeers(
      const CID &cid) const {
    auto maybe_peers_cbored = datastore_->get(cid);
    if (maybe_peers_cbored.has_error()
        && maybe_peers_cbored.error() != IpfsDatastoreError::NOT_FOUND) {
      return maybe_peers_cbored.error();
    }
    if (maybe_peers_cbored.has_error()
        && maybe_peers_cbored.error() == IpfsDatastoreError::NOT_FOUND) {
      return std::vector<RetrievalPeer>{};
    };

    OUTCOME_TRY(peers,
                codec::cbor::decode<std::vector<RetrievalPeerCborable>>(
                    maybe_peers_cbored.value()));
    std::vector<RetrievalPeer> result;
    for (const auto &peer_cborable : peers) {
      OUTCOME_TRY(peer, peer_cborable.toRetrievalPeer());
      result.push_back(peer);
    }
    return std::move(result);
  }

}  // namespace fc::markets::discovery
