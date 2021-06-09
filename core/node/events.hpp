/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/protocol/common/scheduler.hpp>
#include <set>
#include <string>
#include <libp2p/common/metrics/instance_count.hpp>

#include "node/common.hpp"

namespace fc::sync::events {
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
    std::vector<CID> tipset;
    uint64_t height;
    BigInt weight;
  };

  struct BlockFromPubSub {
    PeerId from;
    CID block_cid;
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
    Height height = 0;

    LIBP2P_METRICS_INSTANCE_COUNT(fc::sync::events::PossibleHead);
  };

  struct CurrentHead {
    TipsetCPtr tipset;
    BigInt weight;
  };

  struct FatalError {
    std::string message;
  };

  struct Events : std::enable_shared_from_this<Events> {
    explicit Events(std::shared_ptr<libp2p::protocol::Scheduler> scheduler)
        : scheduler_(std::move(scheduler)) {}

    void stop() {
      stopped_ = true;
    }

#define DEFINE_EVENT(STRUCT)                                             \
  using STRUCT##Callback = void(const STRUCT &);                         \
  Connection subscribe##STRUCT(std::function<STRUCT##Callback> cb) {     \
    return STRUCT##_signal_.connect(cb);                                 \
  }                                                                      \
  void signal##STRUCT(STRUCT event) {                                    \
    scheduler_                                                           \
        ->schedule([wptr = weak_from_this(), event = std::move(event)] { \
          auto self = wptr.lock();                                       \
          if (self && !self->stopped_) {                                 \
            self->STRUCT##_signal_(event);                               \
          }                                                              \
        })                                                               \
        .detach();                                                       \
  }                                                                      \
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
    std::shared_ptr<libp2p::protocol::Scheduler> scheduler_;
    bool stopped_ = false;
  };

}  // namespace fc::sync::events
