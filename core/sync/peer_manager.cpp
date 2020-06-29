/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "peer_manager.hpp"

#include <algorithm>
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
        if (auto str_res = cid.toString()) {
          v.push_back(str_res.value());
          continue;
        }
        log()->error("Cannot convert CID to string");
      }
      return v;
    }

    template <typename T>
    decltype(auto) vecFind(const std::vector<T> &vec, const T &elem) {
      auto it = std::find(vec.begin(), vec.end(), elem);
      bool found = vec.end() != it;
      return std::make_pair(found, it);
    }

  }  // namespace

  PeerManager::PeersRepository::PeerInfoAndProtocols &
  PeerManager::PeersRepository::getRecord(const PeerManager::PeerId &peer_id) {
    auto found = map.find(peer_id);
    if (map.end() == found) {
      list.push_back(peer_id);
      auto peer_iter = list.end();
      std::advance(peer_iter, -1);

      PeerInfoAndProtocols peer_info;
      peer_info.peer_iter = peer_iter;
      map.insert({peer_id, std::move(peer_info)});
    }
    return map[peer_id];
  }

  bool PeerManager::PeersRepository::PeerIdComparator::operator()(
      const PeerManager::PeersRepository::PeersIter &lhs,
      const PeerManager::PeersRepository::PeersIter &rhs) {
    auto lhss = peerToString(*lhs);
    auto rhss = peerToString(*rhs);
    return lhss < rhss;
  }

  const std::string &
  PeerManager::PeersRepository::PeerIdComparator::peerToString(
      const PeerManager::PeerId &peer_id) {
    auto found = cache.find(peer_id);
    if (cache.end() != found) {
      return found->second;
    }
    cache.insert({peer_id, peer_id.toHex()});
    return cache[peer_id];
  }

  PeerManager::PeerManager(
      std::shared_ptr<libp2p::Host> host,
      std::shared_ptr<clock::UTCClock> utc_clock,
      std::shared_ptr<libp2p::protocol::Identify> identify_protocol,
      std::shared_ptr<libp2p::protocol::IdentifyPush> identify_push_protocol,
      std::shared_ptr<libp2p::protocol::IdentifyDelta> identify_delta_protocol)
      : node_protocols_({"/fil/hello/1.0.0",
                         "/ipfs/graphsync/1.0.0",
                         "/ipfs/id/1.0.0",
                         "/ipfs/id/push/1.0.0",
                         "/ipfs/ping/1.0.0",
                         "/meshsub/1.0.0",
                         "/p2p/id/delta/1.0.0"}),
        host_(host),
        utc_clock_(utc_clock),
        hello_(std::make_shared<Hello>()),
        identify_protocol_(identify_protocol),
        identify_push_protocol_(identify_push_protocol),
        identify_delta_protocol_(identify_delta_protocol) {}

  PeerManager::~PeerManager() {
    stop();
  }

  boost::optional<libp2p::peer::PeerInfo> PeerManager::getPeerInfo(
      const PeerId &peer_id) const {
    auto it = peers_.map.find(peer_id);
    if (peers_.map.end() == it) {
      return boost::none;
    }
    auto pi = libp2p::peer::PeerInfo{peer_id, {}};
    if (it->second.connect_to) {
      pi.addresses.push_back(it->second.connect_to.value());
    }
    return pi;
  }

  std::vector<libp2p::peer::PeerId> PeerManager::getPeers() {
    std::vector<PeersRepository::PeersIter> our_network_and_all_protocols;
    PeersRepository::PeerIdComparator comp;
    our_network_and_all_protocols.reserve(
        std::min(peers_.our_network.size(), peers_.all_protocols.size()));
    std::sort(peers_.our_network.begin(), peers_.our_network.end(), comp);
    std::sort(peers_.all_protocols.begin(), peers_.all_protocols.end(), comp);
    std::set_intersection(peers_.our_network.begin(),
                          peers_.our_network.end(),
                          peers_.all_protocols.begin(),
                          peers_.all_protocols.end(),
                          std::back_inserter(our_network_and_all_protocols),
                          comp);
    auto &&sel = our_network_and_all_protocols;
    std::sort(sel.begin(), sel.end(), comp);
    std::vector<PeersRepository::PeersIter> online;
    online.reserve(std::min(sel.size(), peers_.online.size()));
    std::sort(peers_.online.begin(), peers_.online.end(), comp);
    std::set_intersection(sel.begin(),
                          sel.end(),
                          peers_.online.begin(),
                          peers_.online.end(),
                          std::back_inserter(online),
                          comp);

    std::vector<libp2p::peer::PeerId> result;
    result.reserve(online.size());
    for (const auto &it : online) {
      result.push_back(*it);
    }
    return result;
  }

  outcome::result<void> PeerManager::start(const CID &genesis_cid,
                                           const Tipset &tipset,
                                           const BigInt &weight,
                                           OnHello on_hello) {
    if (started_) {
      return outcome::success();
    }
    on_hello_ = std::move(on_hello);
    on_identify_ = identify_protocol_->onIdentifyReceived(
        [wptr = weak_from_this()](const PeerId &peer) {
          if (not wptr.expired()) {
            wptr.lock()->onIdentifyReceived(peer);
          } else {
            log()->error(
                "Unable to process new onIdentify events in PeerManager");
          };
        });

    handleProtocol(*host_, *identify_protocol_);
    handleProtocol(*host_, *identify_push_protocol_);
    handleProtocol(*host_, *identify_delta_protocol_);

    identify_protocol_->start();
    identify_push_protocol_->start();
    identify_delta_protocol_->start();

    fc::sync::Hello::Message initial_state;
    initial_state.heaviest_tipset = tipset.key.cids();
    initial_state.heaviest_tipset_height = tipset.height();
    initial_state.heaviest_tipset_weight = weight;

    hello_->start(
        host_,
        utc_clock_,
        genesis_cid,
        initial_state,
        [wptr = weak_from_this()](const PeerId &peer,
                                  outcome::result<Hello::Message> state) {
          if (not wptr.expired()) {
            wptr.lock()->onHello(peer, std::move(state));
          } else {
            log()->error("Unable to process onHello in PeerManager");
          }
        },
        [wptr = weak_from_this()](const PeerId &peer,
                                  outcome::result<uint64_t> result) {
          if (not wptr.expired()) {
            wptr.lock()->onHelloLatencyMessage(peer, std::move(result));
          } else {
            log()->error(
                "Unable to process onHelloLatencyMessage in PeerManager");
          }
        });

    started_ = true;

    return outcome::success();
  }

  void PeerManager::stop() {
    if (started_) {
      started_ = false;
      host_->stop();
      on_identify_.disconnect();
      hello_->stop();
    }
  }

  void PeerManager::onHeadChanged(const Tipset &tipset, const BigInt &weight) {
    hello_->onHeadChanged({tipset.key.cids(), tipset.height(), weight, {}});
  }

  boost::signals2::connection PeerManager::subscribe(
      const std::function<PeerStatusUpdateCallback> &cb) {
    return peer_update_signal_.connect(cb);
  }

  void PeerManager::reportOfflinePeer(const PeerId &peer_id) {
    auto found = peers_.map.find(peer_id);
    if (peers_.map.end() != found) {
      auto peer_iter = found->second.peer_iter;
      auto found_online =
          std::find(peers_.online.begin(), peers_.online.end(), peer_iter);
      if (peers_.online.end() != found_online) {
        peers_.online.erase(found_online);
        postPeerStatus(peer_id);
      }
    }
  }

  void PeerManager::postPeerStatus(const PeerId &peer_id) {
    auto found = peers_.map.find(peer_id);
    if (peers_.map.end() != found) {
      auto peer_iter = found->second.peer_iter;
      bool online = vecFind(peers_.online, peer_iter).first;
      bool all_protocols = vecFind(peers_.all_protocols, peer_iter).first;
      bool belongs_our_network = vecFind(peers_.our_network, peer_iter).first;
      peer_update_signal_(peer_id, online, all_protocols, belongs_our_network);
    }
  }

  void PeerManager::onIdentifyReceived(const PeerId &peer_id) {
    const std::string bad_addr = "0.0.0.0";
    auto &&peer_info = peers_.getRecord(peer_id);
    auto found = vecFind(peers_.our_network, peer_info.peer_iter);
    if (found.first) {
      peers_.our_network.erase(found.second);
    }
    auto addresses_res =
        host_->getPeerRepository().getAddressRepository().getAddresses(peer_id);

    std::stringstream debug_sstr;
    debug_sstr << "\naddresses: ";

    if (addresses_res) {
      auto &&addrs = addresses_res.value();
      peer_info.connect_to = boost::none;  // after if(res)
      for (const auto &addr : addrs) {
        auto string_addr = addr.getStringAddress();
        debug_sstr << string_addr << ' ';
        if (string_addr.find(bad_addr) != std::string::npos) {
          continue;
        }
        if (not peer_info.connect_to) {
          peer_info.connect_to = addr;
        }
      }
    } else {
      debug_sstr << addresses_res.error().message();
    }

    auto protocols_res =
        host_->getPeerRepository().getProtocolRepository().getProtocols(
            peer_id);
    debug_sstr << "\nprotocols: ";
    peer_info.protocols.clear();  // before if(res)
    if (protocols_res) {
      auto &&protocols = protocols_res.value();
      for (const auto &protocol : protocols) {
        debug_sstr << protocol << ' ';
        peer_info.protocols.push_back(protocol);
      }
    } else {
      debug_sstr << protocols_res.error().message();
    }

    std::vector<std::string> intersection;
    intersection.reserve(node_protocols_.size());
    std::set_intersection(node_protocols_.begin(),
                          node_protocols_.end(),
                          peer_info.protocols.begin(),
                          peer_info.protocols.end(),
                          std::back_inserter(intersection));
    bool all_protocols_supported =
        intersection.size() == node_protocols_.size();

    log()->info(
        "Peer identify for {}: {}", peer_id.toBase58(), debug_sstr.str());

    peers_.online.push_back(peer_info.peer_iter);
    if (all_protocols_supported) {
      peers_.all_protocols.push_back(peer_info.peer_iter);
    }
    postPeerStatus(peer_id);

    if (all_protocols_supported) {
      hello_->sayHello(peer_id);
    }
  }

  void PeerManager::onHello(
      const libp2p::peer::PeerId &peer_id,
      fc::outcome::result<fc::sync::Hello::Message> hello_message) {
    if (not hello_message) {
      log()->info("hello feedback failed for peer {}: {}",
                  peer_id.toBase58(),
                  hello_message.error().message());
      return;
    }
    auto &&peer_info = peers_.getRecord(peer_id);
    peers_.our_network.push_back(peer_info.peer_iter);
    auto &s = hello_message.value();
    log()->info("hello feedback from peer:{}, cids:{}, height:{}, weight:{}",
                peer_id.toBase58(),
                fmt::join(toStrings(s.heaviest_tipset), ","),
                s.heaviest_tipset_height,
                s.heaviest_tipset_weight.str());

    peer_info.current_weight = s.heaviest_tipset_weight;
    postPeerStatus(peer_id);
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

  std::unordered_map<libp2p::peer::PeerId, std::string>
      PeerManager::PeersRepository::PeerIdComparator::cache;

}  // namespace fc::sync
