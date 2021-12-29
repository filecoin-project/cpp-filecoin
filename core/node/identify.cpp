/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "node/identify.hpp"

#include <common/logger.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/protocol/identify.hpp>

#include "node/events.hpp"

namespace fc::sync {

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("identify");
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

  }  // namespace

  Identify::Identify(
      std::shared_ptr<libp2p::Host> host,
      std::shared_ptr<libp2p::protocol::Identify> identify_protocol,
      std::shared_ptr<libp2p::protocol::IdentifyPush> identify_push_protocol,
      std::shared_ptr<libp2p::protocol::IdentifyDelta> identify_delta_protocol)
      : host_(std::move(host)),
        identify_protocol_(std::move(identify_protocol)),
        identify_push_protocol_(std::move(identify_push_protocol)),
        identify_delta_protocol_(std::move(identify_delta_protocol)) {
    assert(host_);
    assert(identify_protocol_);
    assert(identify_push_protocol_);
    assert(identify_delta_protocol_);
  }

  void Identify::start(std::shared_ptr<events::Events> events) {
    events_ = std::move(events);

    assert(events_);

    on_identify_ = identify_protocol_->onIdentifyReceived(
        [wptr = weak_from_this()](const PeerId &peer) {
          if (not wptr.expired()) {
            wptr.lock()->onIdentifyReceived(peer);
          }
        });

    on_disconnect_ =
        host_->getBus()
            .getChannel<libp2p::event::network::OnPeerDisconnectedChannel>()
            .subscribe([this](const PeerId &peer) {
              events_->signalPeerDisconnected({peer});
            });

    handleProtocol(*host_, *identify_protocol_);
    handleProtocol(*host_, *identify_push_protocol_);
    handleProtocol(*host_, *identify_delta_protocol_);

    identify_protocol_->start();
    identify_push_protocol_->start();
    identify_delta_protocol_->start();

    log()->debug("started");
  }

  void Identify::onIdentifyReceived(const PeerId &peer_id) {
    auto protocols_res =
        host_->getPeerRepository().getProtocolRepository().getProtocols(
            peer_id);
    if (!protocols_res || protocols_res.value().empty()) {
      log()->debug("cannot ger protocols for peer {}", peer_id.toBase58());
      return;
    }

    auto &protocols = protocols_res.value();

    log()->debug(
        "peer {} handles {}", peer_id.toBase58(), fmt::join(protocols, ", "));

    events_->signalPeerConnected(events::PeerConnected{
        .peer_id = peer_id, .protocols = {protocols.begin(), protocols.end()}});
  }

}  // namespace fc::sync
