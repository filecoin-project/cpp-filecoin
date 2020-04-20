/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "data_transfer/impl/stream_message_sender.hpp"

#include "codec/cbor/cbor.hpp"

namespace fc::data_transfer {

  StreamMessageSender::StreamMessageSender(
      std::shared_ptr<libp2p::connection::Stream> stream)
      : stream_{std::move(stream)} {}

  outcome::result<void> StreamMessageSender::sendMessage(
      const DataTransferMessage &message) {
    OUTCOME_TRY(encoded_message, codec::cbor::encode(message));
    stream_->write(encoded_message,
                   encoded_message.size(),
                   [](outcome::result<size_t> rwrite) {
                     // nothing;
                   });
    return outcome::success();
  }

  outcome::result<void> StreamMessageSender::close() {
    stream_->close([stream{stream_}](outcome::result<void>) {});
    return outcome::success();
  }

  outcome::result<void> StreamMessageSender::reset() {
    stream_->reset();
    return outcome::success();
  }

}  // namespace fc::data_transfer
