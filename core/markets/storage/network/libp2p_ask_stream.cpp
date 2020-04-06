/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/network/libp2p_ask_stream.hpp"
#include "codec/cbor/cbor.hpp"

namespace fc::markets::storage {

  Libp2pAskStream::Libp2pAskStream(const PeerId &peer_id,
                                   std::shared_ptr<Stream> stream)
      : peer_id_{peer_id}, stream_{std::move(stream)} {}

  outcome::result<AskRequest> Libp2pAskStream::readAskRequest() {
    std::array<uint8_t, buffer_size> buffer{0};
    stream_->read(buffer, buffer_size, [this](outcome::result<size_t> res) {
      if (!res) {
        this->logger_->error("Libp2pAskStream::readAskRequest error: "
                             + res.error().message());
      }
    });
    OUTCOME_TRY(ask_request, codec::cbor::decode<AskRequest>(buffer));
    return std::move(ask_request);
  }

  outcome::result<void> Libp2pAskStream::writeAskRequest(
      const AskRequest &ask_request) {
    OUTCOME_TRY(encoded, codec::cbor::encode(ask_request));
    stream_->write(
        encoded, encoded.size(), [this](outcome::result<size_t> res) {
          if (!res) {
            this->logger_->error("Libp2pAskStream::writeAskRequest error: "
                                 + res.error().message());
          }
        });
    return outcome::success();
  }

  outcome::result<AskResponse> Libp2pAskStream::readAskResponse() {
    std::array<uint8_t, buffer_size> buffer{0};
    stream_->read(buffer, buffer_size, [this](outcome::result<size_t> res) {
      if (!res) {
        this->logger_->error("Libp2pAskStream::readAskResponse error: "
                             + res.error().message());
      }
    });
    OUTCOME_TRY(ask_response, codec::cbor::decode<AskResponse>(buffer));
    return std::move(ask_response);
  }

  outcome::result<void> Libp2pAskStream::writeAskResponse(
      const AskResponse &ask_response) {
    OUTCOME_TRY(encoded, codec::cbor::encode(ask_response));
    stream_->write(
        encoded, encoded.size(), [this](outcome::result<size_t> res) {
          if (!res) {
            this->logger_->error("Libp2pAskStream::writeAskResponse error: "
                                 + res.error().message());
          }
        });
    return outcome::success();
  }

  outcome::result<void> Libp2pAskStream::close() {
    stream_->close([this](outcome::result<void> res) {
      if (!res) {
        this->logger_->error("Libp2pAskStream::close error: "
                             + res.error().message());
      }
    });
    return outcome::success();
  }

}  // namespace fc::markets::storage
