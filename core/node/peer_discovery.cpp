/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "node/peer_discovery.hpp"

#include <stdlib.h>
#include <time.h>
#include <libp2p/host/host.hpp>
#include <libp2p/protocol/kademlia/kademlia.hpp>
#include <libp2p/protocol/kademlia/node_id.hpp>

#include "common/logger.hpp"
#include "node/events.hpp"

namespace fc::sync {

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("peer_discovery");
      return logger.get();
    }

    // Hardcoded config values at the moment

    // random walk periodic timer
    constexpr std::chrono::milliseconds kTimerPeriodMsec =
        std::chrono::milliseconds(120000);

    // Enough # of connections, will not initiate new connections while
    // n_connections_ is above
    constexpr size_t kEnoughConnectionsNum = 100;

    // Iterations to choose random peer id close enough to ours in DHT sence
    constexpr size_t kRandomPeerIterations = 12;
  }  // namespace

  PeerDiscovery::PeerDiscovery(
      std::shared_ptr<libp2p::Host> host,
      std::shared_ptr<Scheduler> scheduler,
      std::shared_ptr<libp2p::protocol::kademlia::Kademlia> kademlia)
      : host_(std::move(host)),
        scheduler_(std::move(scheduler)),
        kademlia_(std::move(kademlia)) {
    assert(host_);
    assert(scheduler_);
    assert(kademlia_);
  }

  void PeerDiscovery::start(events::Events &events) {
    this_node_id_ =
        libp2p::protocol::kademlia::NodeId(host_->getId()).getData();

    peer_connected_event_ = events.subscribePeerConnected(
        [this](const events::PeerConnected &) { ++n_connections_; });

    peer_disconnected_event_ = events.subscribePeerDisconnected(
        [this](const events::PeerDisconnected &) {
          if (n_connections_ > 0) --n_connections_;
        });

    block_pubsub_event_ = events.subscribeBlockFromPubSub(
        [this](const events::BlockFromPubSub &e) {
          onPossibleConnection(e.from);
        });

    message_pubsub_event_ = events.subscribeMessageFromPubSub(
        [this](const events::MessageFromPubSub &e) {
          onPossibleConnection(e.from);
        });

    timer_handle_ = scheduler_->scheduleWithHandle([this] { onTimer(); });

    kademlia_->start();

    // we dont need heavy and proper RNG here for random walks
    srand(time(0));

    log()->debug("started");
  }

  void PeerDiscovery::makeRequest(libp2p::peer::PeerId peer_id) {
    auto res = kademlia_->findPeer(
        peer_id, [this](outcome::result<libp2p::peer::PeerInfo> pi) {
          if (!pi) {
            log()->debug("kademlia called back with error: {}",
                         pi.error().message());
            requests_in_progress_.clear();
          } else {
            onPeerResolved(pi.value());
          }
        });

    if (!res) {
      log()->error("kademlia find peer returned: {}", res.error().message());
      requests_in_progress_.clear();
    } else {
      requests_in_progress_.insert(std::move(peer_id));
    }
  }

  void PeerDiscovery::onPossibleConnection(
      const libp2p::peer::PeerId &peer_id) {
    if (requests_in_progress_.count(peer_id) > 0) {
      // dont make extra requests
      return;
    }
    auto res =
        host_->getPeerRepository().getAddressRepository().getAddresses(peer_id);
    if (res && !res.value().empty()) {
      // host is resolved, ignore
      return;
    }
    makeRequest(peer_id);
  }

  void PeerDiscovery::onPeerResolved(const libp2p::peer::PeerInfo &peer_info) {
    log()->debug("resolved address(es) for id={}", peer_info.id.toBase58());
    requests_in_progress_.erase(peer_info.id);
    if (n_connections_ < kEnoughConnectionsNum) {
      host_->connect(peer_info);
    }
  }

  namespace {
    PeerId genRandomPeer() {
      // should be large enough for PeerId to take hash from it, not "identity"
      static constexpr size_t kPseudoKeySize = 49;

      std::vector<uint8_t> pseudo_key;
      pseudo_key.reserve(kPseudoKeySize);
      for (size_t i = 0; i < kPseudoKeySize; ++i) {
        // yes, random walk doesnt need any good RNG, this is enough
        pseudo_key.push_back((uint8_t)(rand() % 256));
      }

      return PeerId::fromPublicKey(
                 libp2p::crypto::ProtobufKey(std::move(pseudo_key)))
          .value();
    }
  }  // namespace

  void PeerDiscovery::onTimer() {
    libp2p::common::Hash256 min_distance;
    memset(min_distance.data(), 0xFF, min_distance.size());

    // random ID but close enough to ours in DHT sense
    boost::optional<PeerId> peer_id;

    for (size_t i = 0; i < kRandomPeerIterations; ++i) {
      auto random_id = genRandomPeer();
      auto node_id = libp2p::protocol::kademlia::NodeId(random_id);
      auto distance = libp2p::protocol::kademlia::xor_distance(
          this_node_id_, node_id.getData());
      if (distance < min_distance) {
        min_distance = distance;
        peer_id = std::move(random_id);
      }
    }

    log()->debug("chose new random walk peer {}", peer_id->toBase58());

    makeRequest(peer_id.value());

    timer_handle_ =
        scheduler_->scheduleWithHandle([this] { onTimer(); }, kTimerPeriodMsec);
  }

}  // namespace fc::sync
