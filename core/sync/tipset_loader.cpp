/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tipset_loader.hpp"

#include <cassert>

namespace fc::sync {
  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("sync");
      return logger.get();
    }
  }  // namespace

  TipsetLoader::TipsetLoader(
      std::shared_ptr<libp2p::protocol::Scheduler> scheduler,
      std::shared_ptr<BlockLoader> block_loader)
      : scheduler_(std::move(scheduler)),
        block_loader_(std::move(block_loader)) {
    assert(scheduler_);
    assert(block_loader_);
  }

  void TipsetLoader::init(OnTipset callback) {
    assert(callback);
    block_loader_->init(
        [this](const CID &cid, outcome::result<BlockHeader> bh) {
          onBlock(cid, std::move(bh));
        });
    callback_ = std::move(callback);
    initialized_ = true;
  }

  outcome::result<void> TipsetLoader::loadTipsetAsync(
      const TipsetKey &key,
      boost::optional<PeerId> preferred_peer, uint64_t depth) {
    if (!initialized_) {
      return Error::SYNC_NOT_INITIALIZED;
    }

    if (tipset_requests_.count(key.hash()) != 0) {
      // already waiting, do nothing
      return outcome::success();
    }

    OUTCOME_TRY(blocks_available,
                block_loader_->loadBlocks(key.cids(), preferred_peer, depth));

    size_t n = key.cids().size();

    assert(blocks_available.size() == key.cids().size());

    Wantlist wantlist;

    for (size_t i = 0; i < n; ++i) {
      if (!blocks_available[i].has_value()) {
        wantlist.insert(key.cids()[i]);
      }
    }

    if (wantlist.empty()) {
      auto res = Tipset::create(key.hash(), std::move(blocks_available));
      if (!res) {
        log()->error("TipsetLoader: cannot create tipset, err=",
                     res.error().message());
        return Error::SYNC_BAD_TIPSET;
      }

      scheduler_
          ->schedule(
              [wptr = weak_from_this(), res = std::move(res)]() {
                auto self = wptr.lock();
                if (self) {
                  self->callback_(res.value()->key.hash(), std::move(res));
                }
              })
          .detach();

      return outcome::success();
    }

    global_wantlist_.insert(wantlist.begin(), wantlist.end());

    RequestCtx ctx(*this, key);
    ctx.wantlist = std::move(wantlist);
    ctx.blocks_filled = std::move(blocks_available);

    tipset_requests_.insert({key.hash(), std::move(ctx)});
    return outcome::success();
  }

  TipsetLoader::RequestCtx::RequestCtx(TipsetLoader &o, const TipsetKey &key)
      : owner(o), tipset_key(key) {}

  void TipsetLoader::RequestCtx::onBlockSynced(const CID &cid,
                                               const BlockHeader &bh) {
    if (is_bad_tipset) {
      return;
    }

    auto it = wantlist.find(cid);
    if (it == wantlist.end()) {
      // not our block
      return;
    }

    wantlist.erase(it);

    const auto &cids = tipset_key.cids();
    size_t pos = 0;
    for (; pos < cids.size(); ++pos) {
      if (cids[pos] == cid) break;
    }

    assert(pos <= cids.size());

    blocks_filled[pos] = bh;

    if (!wantlist.empty()) {
      // wait for remaining blocks
      return;
    }

    call_completed = owner.scheduler_->schedule([this]() {
      auto res = Tipset::create(tipset_key.hash(), std::move(blocks_filled));
      owner.onRequestCompleted(tipset_key.hash(), std::move(res));
    });
  }

  void TipsetLoader::RequestCtx::onError(const CID &cid) {
    if (is_bad_tipset) {
      return;
    }

    auto it = wantlist.find(cid);
    if (it == wantlist.end()) {
      // not our block
      return;
    }

    is_bad_tipset = true;

    call_completed = owner.scheduler_->schedule([this]() {
      owner.onRequestCompleted(tipset_key.hash(), Error::SYNC_BAD_TIPSET);
    });
  }

  void TipsetLoader::onRequestCompleted(TipsetHash hash,
                                        outcome::result<TipsetCPtr> tipset) {
    tipset_requests_.erase(hash);
    callback_(std::move(hash), std::move(tipset));
  }

  void TipsetLoader::onBlock(const CID &cid, outcome::result<BlockHeader> bh) {
    auto it = global_wantlist_.find(cid);
    if (it == global_wantlist_.end()) {
      // not our block
      return;
    }

    global_wantlist_.erase(cid);

    if (bh) {
      const auto &value = bh.value();
      for (auto &[_, ctx] : tipset_requests_) {
        ctx.onBlockSynced(cid, value);
      }
    } else {
      for (auto &[_, ctx] : tipset_requests_) {
        ctx.onError(cid);
      }
    }
  }

}  // namespace fc::sync
