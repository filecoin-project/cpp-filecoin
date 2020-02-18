/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "connector.hpp"
#include "../session.hpp"

namespace fc::storage::ipfs::graphsync {

  Connector::Connector(std::shared_ptr<libp2p::Host> host,
                       libp2p::peer::Protocol protocol_id,
                       OnSessionConnected callback)
      : host_(std::move(host)),
        protocol_id_(std::move(protocol_id)),
        callback_(std::move(callback)) {
    assert(host_);
    assert(callback_);
  }

  void Connector::tryConnect(SessionPtr session) {
    libp2p::peer::PeerInfo pi{session->peer, {}};
    if (session->connect_to) {
      pi.addresses.push_back(session->connect_to.value());
    }

    // clang-format off
    host_->newStream(
        pi,
        protocol_id_,
        [wptr{weak_from_this()}, s=std::move(session)] (auto rstream) {
          auto self = wptr.lock();
          if (self) {
            self->onStreamConnected(s, std::move(rstream));
          }
        }
    );
    // clang-format on
  }

  void Connector::onStreamConnected(
      SessionPtr session, libp2p::outcome::result<StreamPtr> rstream) {
    if (rstream) {
      session->stream = std::move(rstream.value());
      return callback_(std::move(session), outcome::success());
    }

    callback_(std::move(session), rstream.error());
  }

}  // namespace fc::storage::ipfs::graphsync
