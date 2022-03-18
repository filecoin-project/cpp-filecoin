/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "node/head_constructor.hpp"

#include "common/logger.hpp"
#include "node/events.hpp"

namespace fc::sync {

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("head_constructor");
      return logger.get();
    }
  }  // namespace

  void HeadConstructor::start(std::shared_ptr<events::Events> events) {
    events_ = std::move(events);
    assert(events_);

    block_from_pubsub_event_ = events_->subscribeBlockFromPubSub(
        [this](const events::BlockFromPubSub &e) {
          tryAppendBlock(e.from, e.block_cid, e.block.header);
        });

    tipset_from_hello_event_ = events_->subscribeTipsetFromHello(
        [this](const events::TipsetFromHello &e) {
          if (e.height < current_height_) {
            log()->debug("ignoring head from peer {} with height {}<{}",
                         e.peer_id.toBase58(),
                         e.height,
                         current_height_);
            return;
          }
          if (e.weight <= min_weight_) {
            log()->debug("ignoring head from peer {} with weight {}<={}",
                         e.peer_id.toBase58(),
                         e.weight,
                         min_weight_);
            return;
          }
          log()->debug("possible head from {}, weight={}, height={}",
                       e.peer_id.toBase58(),
                       e.weight,
                       e.height);
          events_->signalPossibleHead({.source = e.peer_id,
                                       .head = TipsetKey(e.tipset),
                                       .height = e.height});
        });

    current_head_event_ =
        events_->subscribeCurrentHead([this](const events::CurrentHead &e) {
          if (e.weight > min_weight_) min_weight_ = e.weight;
        });
  }

  void HeadConstructor::blockFromApi(const CbCid &block_cid,
                                     const BlockHeader &block) {
    tryAppendBlock(boost::none, block_cid, block);
  }

  void HeadConstructor::tryAppendBlock(boost::optional<PeerId> source,
                                       const CbCid &block_cid,
                                       const BlockHeader &header) {
    if (header.height < current_height_) {
      log()->warn("ignoring block from {} with height {}<{}",
                  source.has_value() ? source->toBase58() : "API",
                  header.height,
                  current_height_);
      return;
    }

    log()->debug("new block from {}, height={}",
                 source.has_value() ? source->toBase58() : "API",
                 header.height);

    if (header.height > current_height_) {
      // TODO(artem): ignore height from future epoch, use ChainEpochClock (?)

      current_height_ = header.height;
      log()->debug("switching to height {}", current_height_);
      candidates_.clear();
    }

    auto &creator = candidates_[TipsetKey::hash(header.parents)];
    auto res = creator.canExpandTipset(header);
    if (!res) {
      log()->error("cannot expand tipset with new block, {}",
                   res.error().message());
      return;
    }
    res = creator.expandTipset(block_cid, header);
    if (!res) {
      log()->error("cannot expand tipset with new block, {}",
                   res.error().message());
      return;
    }

    log()->debug("new possible head");

    // TODO check if parents are already synced / downloaded

    events_->signalPossibleHead({source, creator.key(), creator.height()});
  }

}  // namespace fc::sync
