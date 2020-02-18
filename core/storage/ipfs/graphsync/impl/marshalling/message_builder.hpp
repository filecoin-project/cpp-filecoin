/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_MESSAGE_BUILDER_HPP
#define CPP_FILECOIN_GRAPHSYNC_MESSAGE_BUILDER_HPP

#include <gsl/span>

#include <libp2p/outcome/outcome.hpp>

#include "storage/ipfs/graphsync/impl/message.hpp"

namespace fc::storage::ipfs::graphsync {

  namespace pb {
    // fwd declaration of protobuf message
    class Message;
  }

  /// Base class for request and response builders
  class MessageBuilder {
   public:
    MessageBuilder();
    virtual ~MessageBuilder();

    bool empty() const;

    size_t getSerializedSize() const;

    outcome::result<SharedData> serialize();

    void clear();

   protected:
    std::unique_ptr<pb::Message> pb_msg_; //NOLINT
    bool empty_ = true; //NOLINT
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_MESSAGE_BUILDER_HPP
