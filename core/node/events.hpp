/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio/io_context.hpp>
#include <set>
#include <string>

#include "node/common.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"

namespace fc::sync::events {
  using primitives::ChainEpoch;

  struct PeerConnected {
    PeerId peer_id;
    std::set<std::string> protocols;
  };

  struct PeerDisconnected {
    PeerId peer_id;
  };

  struct PeerLatency {
    PeerId peer_id;
    uint64_t latency_usec;
  };

  struct TipsetFromHello {
    PeerId peer_id;
    std::vector<CbCid> tipset;
    ChainEpoch height;
    BigInt weight;
  };

  struct BlockFromPubSub {
    PeerId from;
    CbCid block_cid;
    BlockWithCids block;
  };

  struct MessageFromPubSub {
    PeerId from;
    CID cid;
    SignedMessage msg;
  };

  struct PossibleHead {
    boost::optional<PeerId> source;
    TipsetKey head;
    ChainEpoch height = 0;
  };

  struct CurrentHead {
    TipsetCPtr tipset;
    BigInt weight;
  };

  struct FatalError {
    std::string message;
  };

  struct Events : std::enable_shared_from_this<Events> {
    explicit Events(std::shared_ptr<boost::asio::io_context> io_context)
        : io_context_(std::move(io_context)) {}

    void stop() {
      stopped_ = true;
    }

#define DEFINE_EVENT(STRUCT)                                                \
  using STRUCT##Callback = void(const STRUCT &);                            \
  Connection subscribe##STRUCT(std::function<STRUCT##Callback> cb) {        \
    return STRUCT##_signal_.connect(cb);                                    \
  }                                                                         \
  void signal##STRUCT(STRUCT event) {                                       \
    io_context_->post([wptr = weak_from_this(), event = std::move(event)] { \
      auto self = wptr.lock();                                              \
      if (self && !self->stopped_) {                                        \
        self->STRUCT##_signal_(event);                                      \
      }                                                                     \
    });                                                                     \
  }                                                                         \
  boost::signals2::signal<STRUCT##Callback> STRUCT##_signal_

    DEFINE_EVENT(PeerConnected);
    DEFINE_EVENT(PeerDisconnected);
    DEFINE_EVENT(PeerLatency);
    DEFINE_EVENT(TipsetFromHello);
    DEFINE_EVENT(CurrentHead);
    DEFINE_EVENT(BlockFromPubSub);
    DEFINE_EVENT(MessageFromPubSub);
    DEFINE_EVENT(PossibleHead);
    DEFINE_EVENT(FatalError);

#undef DEFINE_EVENT

   private:
    std::shared_ptr<boost::asio::io_context> io_context_;
    bool stopped_ = false;
  };

}  // namespace fc::sync::events
