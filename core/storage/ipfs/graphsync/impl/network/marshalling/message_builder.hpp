/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "message.hpp"

namespace fc::storage::ipfs::graphsync {

  namespace pb {
    // fwd declaration of protobuf message
    class Message;
  }

  /// Base class for request and response builders
  class MessageBuilder {
   public:
    MessageBuilder();
    MessageBuilder(const MessageBuilder &) = delete;
    MessageBuilder(MessageBuilder &&) = delete;
    virtual ~MessageBuilder();

    const MessageBuilder &operator=(const MessageBuilder &) = delete;
    const MessageBuilder &operator=(MessageBuilder &&) = delete;

    /// Returns if there is nothing to send
    bool empty() const;

    /// Returns serialized size of protobuf message
    size_t getSerializedSize() const;

    /// Serializes message to shared byte buffer
    outcome::result<SharedData> serialize();

    /// Clears all entries added
    virtual void clear();

   protected:
    /// Protobuf message, reused by derived classes
    std::unique_ptr<pb::Message> pb_msg_; //NOLINT

    /// Empty message indicator, reused by derived classes
    bool empty_ = true; //NOLINT
  };

}  // namespace fc::storage::ipfs::graphsync
