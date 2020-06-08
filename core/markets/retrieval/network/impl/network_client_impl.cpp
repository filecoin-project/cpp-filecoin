/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/retrieval/network/impl/network_client_impl.hpp"

namespace fc::markets::retrieval::network {
  outcome::result<NetworkClient::StreamShPtr> NetworkClientImpl::connect(
      const PeerInfo &peer, const Protocol &proto) {
    StreamShPtr stream{};
    OUTCOME_TRY(Async::run([this, &peer, &proto, &stream](Operation operation) {
      this->openStream(operation, peer, proto, stream);
    }));
    return stream;
  }

  void NetworkClientImpl::openStream(Operation operation,
                                     const PeerInfo &peer,
                                     const Protocol &proto,
                                     StreamShPtr &stream) {
    host_service_->newStream(
        peer, proto, [operation, &stream](outcome::result<StreamShPtr> result) {
          if (result.has_value()) {
            stream = std::move(result.value());
            operation->set_value();
          } else {
            Async::failure(operation, NetworkClientError::ConnectionError);
          }
        });
  }

}  // namespace fc::markets::retrieval::network

OUTCOME_CPP_DEFINE_CATEGORY(fc::markets::retrieval::network,
                            NetworkClientError,
                            e) {
  using Error = fc::markets::retrieval::network::NetworkClientError;
  switch (e) {
    case Error::ConnectionError:
      return "Failed to connect to a remote peer";
    case Error::NetworkError:
      return "Unknown network error";
  }
}
