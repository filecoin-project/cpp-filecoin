/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "peer_manager.hpp"

#include <sstream>

#include <common/logger.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/protocol/identify.hpp>

#include "storage/chain/chain_store.hpp"

namespace fc::sync {

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("sync");
      return logger.get();
    }

    void handleProtocol(libp2p::Host &host,
                        libp2p::protocol::BaseProtocol &protocol) {
      host.setProtocolHandler(
          protocol.getProtocolId(),
          [&protocol](libp2p::protocol::BaseProtocol::StreamResult res) {
            protocol.handle(std::move(res));
          });
    }

    std::vector<std::string> toStrings(const std::vector<fc::CID> &cids) {
      std::vector<std::string> v;
      v.reserve(cids.size());
      for (const auto &cid : cids) {
        v.push_back(cid.toString().value());
      }
      return v;
    }

  }  // namespace

  PeerManager::PeerManager(const node::NodeObjects &o, const node::Config &c)
      : node_protocols_({"/fil/hello/1.0.0",
                         "/ipfs/graphsync/1.0.0",
                         "/ipfs/id/1.0.0",
                         "/ipfs/id/push/1.0.0",
                         "/ipfs/ping/1.0.0",
                         "/meshsub/1.0.0",
                         "/p2p/id/delta/1.0.0"}),
        host_(o.host),
        hello_(std::make_shared<Hello>()),
        identify_protocol_(o.identify_protocol),
        identify_push_protocol_(o.identify_push_protocol),
        identify_delta_protocol_(o.identify_delta_protocol),
        chain_store_(o.chain_store) {
    auto id = host_->getId();
    for (const auto &peer : c.bootstrap_list) {
      if (id != peer.id) {
        bootstrap_peers_.push_back(peer);
      }
    }
  }

  PeerManager::~PeerManager() {
    stop();
  }

  boost::optional<libp2p::peer::PeerInfo> PeerManager::getPeerInfo(
      const PeerId &peer_id) const {
    auto it = node_peers_.find(peer_id);
    if (it == node_peers_.end()) {
      return boost::none;
    }
    auto pi = libp2p::peer::PeerInfo{it->first, {}};
    if (it->second.connect_to) {
      pi.addresses.push_back(it->second.connect_to.value());
    }
    return pi;
  }

  const std::vector<libp2p::peer::PeerInfo> &PeerManager::getBootstrapPeers()
      const {
    return bootstrap_peers_;
  }

  boost::optional<libp2p::peer::PeerId> PeerManager::choosePeer() {
    while (!node_peers_weighted_.empty()) {
      const auto &it = node_peers_weighted_.begin();
      if (node_peers_.count(it->second)) {
        return it->second;
      } else {
        node_peers_weighted_.erase(it);
      }
    }
    return boost::none;
  }

  outcome::result<void> PeerManager::start() {
    if (started_) {
      return outcome::success();
    }

    auto tipset_res = chain_store_->heaviestTipset();
    if (!tipset_res) {
      return tipset_res.error();
    }
    auto genesis_res = chain_store_->getGenesis();
    if (!genesis_res) {
      return genesis_res.error();
    }
    auto genesis_cid_res = primitives::cid::getCidOfCbor(genesis_res.value());
    if (!genesis_cid_res) {
      return genesis_cid_res.error();
    }
    CID genesis_cid = std::move(genesis_cid_res.value());

    on_identify_ = identify_protocol_->onIdentifyReceived(
        [wptr = weak_from_this()](const PeerId &peer) {
          auto sp = wptr.lock();
          if (sp) sp->onIdentifyReceived(peer);
        });

    handleProtocol(*host_, *identify_protocol_);
    handleProtocol(*host_, *identify_push_protocol_);
    handleProtocol(*host_, *identify_delta_protocol_);

    identify_protocol_->start();
    identify_push_protocol_->start();
    identify_delta_protocol_->start();

    fc::sync::Hello::Message initial_state;
    initial_state.heaviest_tipset = tipset_res.value().cids;
    initial_state.heaviest_tipset_height = tipset_res.value().height;
    initial_state.heaviest_tipset_weight = chain_store_->getHeaviestWeight();

    hello_->start(
        host_,
        utc_clock_,
        genesis_cid_res.value(),
        initial_state,
        [wptr = weak_from_this()](const PeerId &peer,
                                  outcome::result<Hello::Message> state) {
          auto sp = wptr.lock();
          if (sp) sp->onHello(peer, std::move(state));
        },
        [wptr = weak_from_this()](const PeerId &peer,
                                  outcome::result<uint64_t> result) {
          auto sp = wptr.lock();
          if (sp) sp->onHelloLatencyMessage(peer, std::move(result));
        });

    host_->start();

    started_ = true;

    for (const auto &pi : bootstrap_peers_) {
      host_->connect(pi);
    }

    return outcome::success();
  }

  void PeerManager::stop() {
    if (started_) {
      started_ = false;
      host_->stop();

      hello_->stop();
    }
  }

  void PeerManager::onIdentifyReceived(const PeerId &peer) {
    auto addr_res =
        host_->getPeerRepository().getAddressRepository().getAddresses(peer);

    std::stringstream sstr;
    InfoAndProtocols info;
    auto needed_protocol_set = node_protocols_;

    sstr << "\naddresses: ";
    if (addr_res) {
      for (const auto &addr : addr_res.value()) {
        sstr << addr.getStringAddress() << ' ';
      }
      if (!addr_res.value().empty()) {
        info.connect_to = addr_res.value().at(0);

        // TODO: substitute remote peer's host instead of they sent
      }
    } else {
      sstr << addr_res.error().message();
    }

    auto proto_res =
        host_->getPeerRepository().getProtocolRepository().getProtocols(peer);

    sstr << "\nprotocols: ";
    if (proto_res) {
      for (const auto &proto : proto_res.value()) {
        sstr << proto << ' ';
        info.protocols.push_back(proto);
        needed_protocol_set.erase(proto);
      }
    } else {
      sstr << proto_res.error().message() << '\n';
    }

    bool is_node_candidate = needed_protocol_set.empty();

    if (is_node_candidate) {
      sstr << "\nrequired protocol set is there";
    } else {
      sstr << "\nmissing protocols: ";
      for (const auto &proto : needed_protocol_set) {
        sstr << proto << ' ';
      }
    }

    log()->info("Peer identify for {}: {}", peer.toBase58(), sstr.str());

    other_peers_[peer] = std::move(info);
    if (is_node_candidate) {
      hello_->sayHello(peer);
    }
  }

  void PeerManager::onHello(
      const libp2p::peer::PeerId &peer,
      fc::outcome::result<fc::sync::Hello::Message> state) {
    if (!state) {
      log()->info("hello feedback failed for peer {}: {}",
                  peer.toBase58(),
                  state.error().message());
      return;
    }

    auto &s = state.value();

    log()->info("hello feedback from peer:{}, cids:{}, height:{}, weight:{}",
                peer.toBase58(),
                fmt::join(toStrings(s.heaviest_tipset), ","),
                s.heaviest_tipset_height,
                s.heaviest_tipset_weight.str());

    auto &info = node_peers_[peer];
    auto it = other_peers_.find(peer);
    if (it != other_peers_.end()) {
      info = std::move(it->second);
      other_peers_.erase(it);
    } else {
      log()->debug("unexpected hello feedback from unidentified peer");
    }
    info.current_weight = s.heaviest_tipset_weight;

  //TODO
  }

  void PeerManager::onHelloLatencyMessage(
      const libp2p::peer::PeerId &peer, fc::outcome::result<uint64_t> result) {
    if (!result) {
      log()->info("latency feedback failed for peer {}: {}",
                  peer.toBase58(),
                  result.error().message());
      return;
    }

    log()->info("latency feedback from peer {}: {} microsec",
                peer.toBase58(),
                result.value() / 1000);
  }

}  // namespace fc::sync
