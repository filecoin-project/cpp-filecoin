/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/network/libp2p_deal_stream.hpp"

namespace fc::markets::storage {

  Libp2pDealStream::Libp2pDealStream(PeerId peer_id,
                                     std::shared_ptr<Stream> stream)
      : peer_id_{std::move(peer_id)}, stream_{std::move(stream)} {}

  outcome::result<Proposal> Libp2pDealStream::readDealProposal() {
    std::array<uint8_t, buffer_size> buffer{0};
    stream_->read(buffer, buffer_size, [this](outcome::result<size_t> res) {
      if (!res) {
        this->logger_->error("Libp2pDealStream::readDealProposal error: "
                             + res.error().message());
      }
    });
    OUTCOME_TRY(proposal, codec::cbor::decode<Proposal>(buffer));
    return std::move(proposal);
  }

  outcome::result<void> Libp2pDealStream::writeDealProposal(
      const Proposal &proposal) {
    OUTCOME_TRY(encoded, codec::cbor::encode(proposal));
    stream_->write(
        encoded, encoded.size(), [this](outcome::result<size_t> res) {
          if (!res) {
            this->logger_->error("Libp2pDealStream::writeDealProposal error: "
                                 + res.error().message());
          }
        });
    return outcome::success();
  }

  outcome::result<SignedResponse> Libp2pDealStream::readDealResponse() {
    std::array<uint8_t, buffer_size> buffer{0};
    stream_->read(buffer, buffer_size, [this](outcome::result<size_t> res) {
      if (!res) {
        this->logger_->error("Libp2pDealStream::readDealResponse error: "
                             + res.error().message());
      }
    });
    OUTCOME_TRY(response, codec::cbor::decode<SignedResponse>(buffer));
    return std::move(response);
  }

  outcome::result<void> Libp2pDealStream::writeDealResponse(
      const SignedResponse &response) {
    OUTCOME_TRY(encoded, codec::cbor::encode(response));
    stream_->write(
        encoded, encoded.size(), [this](outcome::result<size_t> res) {
          if (!res) {
            this->logger_->error("Libp2pDealStream::writeAskResponse error: "
                                 + res.error().message());
          }
        });
    return outcome::success();
  }

  PeerId Libp2pDealStream::remotePeer() const {
    return peer_id_;
  }

  outcome::result<void> Libp2pDealStream::close() {
    stream_->close([this](outcome::result<void> res) {
      if (!res) {
        this->logger_->error("Libp2pDealStream::close error: "
                             + res.error().message());
      }
    });
    return outcome::success();
  }

}  // namespace fc::markets::storage
