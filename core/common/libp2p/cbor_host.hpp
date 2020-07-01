/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_COMMON_LIBP2P_CBOR_HOST_HPP
#define CPP_FILECOIN_CORE_COMMON_LIBP2P_CBOR_HOST_HPP

#include <libp2p/host/host.hpp>
#include "common/libp2p/cbor_stream.hpp"

namespace fc::common::libp2p {
  using ::libp2p::Host;
  using ::libp2p::peer::PeerInfo;
  using ::libp2p::peer::Protocol;
  using NewCborStreamHandler =
      std::function<void(outcome::result<std::shared_ptr<CborStream>>)>;
  using NewCborProtocolHandler =
      std::function<void(std::shared_ptr<CborStream>)>;

  /**
   * libp2p::Host wrapper with cbor stream support
   */
  class CborHost {
   public:
    explicit CborHost(std::shared_ptr<Host> host) : host_{std::move(host)} {}

    void newCborStream(const PeerInfo &peer_info,
                       const Protocol &protocol,
                       const NewCborStreamHandler &handler) {
      host_->newStream(peer_info, protocol, [handler](auto stream) {
        if (stream.has_error()) {
          handler(stream.error());
          return;
        }
        handler(std::make_shared<CborStream>(stream.value()));
      });
    }

    void setCborProtocolHandler(const Protocol &protocol,
                                const NewCborProtocolHandler &handler) {
      host_->setProtocolHandler(protocol, [handler](auto stream) {
        handler(std::make_shared<CborStream>(stream));
      });
    }

    PeerInfo getPeerInfo() const {
      return host_->getPeerInfo();
    }

   private:
    std::shared_ptr<Host> host_;
  };

}  // namespace fc::common::libp2p

#endif  // CPP_FILECOIN_CORE_COMMON_LIBP2P_CBOR_HOST_HPP
