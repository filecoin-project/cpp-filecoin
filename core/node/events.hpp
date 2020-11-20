/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_EVENTS_HPP
#define CPP_FILECOIN_SYNC_EVENTS_HPP

#include "fwd.hpp"

#include <set>
#include <string>

#include <libp2p/protocol/common/scheduler.hpp>

#include "vm/interpreter/interpreter.hpp"

namespace libp2p {
  namespace peer {
    class PeerInfo;
  }  // namespace peer
}  // namespace libp2p

namespace fc {

  namespace primitives {
    namespace tipset {
      struct Tipset;
    }  // namespace tipset
  }    // namespace primitives

}  // namespace fc

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
    int64_t latency_usec;
  };

  struct TipsetFromHello {
    PeerId peer_id;
    std::vector<CID> tipset;
    uint64_t height;
    BigInt weight;
  };

  struct CurrentHead {
    TipsetCPtr tipset;
    BigInt weight;
  };

  struct BlockFromPubSub {
    PeerId from;
    CID block_cid;
    BlockWithCids msg;
  };

  struct MessageFromPubSub {
    PeerId from;
    CID cid;
    UnsignedMessage msg;
    boost::optional<Signature> signature;
  };

  struct BlockStored {
    boost::optional<PeerId> from;
    CID block_cid;
    outcome::result<BlockWithCids> block;
    bool messages_stored = false;
  };

  struct TipsetStored {
    TipsetHash hash;
    outcome::result<TipsetCPtr> tipset;

    // bottom of unsynced chain: tipset whose parents are not yet stored
    boost::optional<TipsetCPtr> proceed_sync_from;
  };

  struct HeadInterpreted {
    TipsetCPtr head;
    outcome::result<vm::interpreter::Result> result;
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
    DEFINE_EVENT(BlockStored);
    DEFINE_EVENT(TipsetStored);
    DEFINE_EVENT(HeadInterpreted);

#undef DEFINE_EVENT

   private:
    std::shared_ptr<libp2p::protocol::Scheduler> scheduler_;
    bool stopped_ = false;
  };

}  // namespace fc::sync::events

#endif  // CPP_FILECOIN_SYNC_EVENTS_HPP
