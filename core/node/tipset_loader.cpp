/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tipset_loader.hpp"

#include "chain_db.hpp"
#include "common/logger.hpp"
#include "events.hpp"

namespace fc::sync {
  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("tipset_loader");
      return logger.get();
    }
  }  // namespace

  TipsetLoader::TipsetLoader(
      std::shared_ptr<blocksync::BlocksyncClient> blocksync,
      std::shared_ptr<ChainDb> chain_db)
      : blocksync_(std::move(blocksync)), chain_db_(std::move(chain_db)) {
    assert(blocksync_);
    assert(chain_db_);
  }

  void TipsetLoader::start(std::shared_ptr<events::Events> events) {
    events_ = std::move(events);
    assert(events_);
    block_stored_event_ = events_->subscribeBlockStored(
        [wptr = weak_from_this()](const events::BlockStored &e) {
          auto self = wptr.lock();
          if (self && self->initialized_) {
            self->onBlock(e);
          }
        });
    initialized_ = true;
  }

  outcome::result<void> TipsetLoader::loadTipsetAsync(
      const TipsetKey &key,
      boost::optional<PeerId> preferred_peer,
      uint64_t depth) {
    if (!initialized_) {
      return Error::TIPSET_LOADER_NOT_INITIALIZED;
    }

    if (tipset_requests_.count(key.hash()) != 0) {
      // already waiting, do nothing
      return outcome::success();
    }

    auto maybe_tipset = chain_db_->getTipsetByKey(key);
    if (maybe_tipset) {
      events::TipsetStored event{.hash = key.hash(),
                                 .tipset = std::move(maybe_tipset.value()),
                                 .proceed_sync_from = boost::none};

      auto res = chain_db_->getUnsyncedBottom(key);
      if (!res) {
        event.tipset = res.error();
      } else {
        event.proceed_sync_from = std::move(res.value());
      }

      events_->signalTipsetStored(std::move(event));

      log()->warn("tipset found locally {}", key.toPrettyString());
      return outcome::success();
    }

    if (preferred_peer.has_value()) {
      // TODO subscribe on peer connected-disconnected events

      if (!last_peer_.has_value()
          || last_peer_.value() != preferred_peer.value()) {
        last_peer_ = std::move(preferred_peer);
      }
    }

    if (!last_peer_.has_value()) {
      return Error::TIPSET_LOADER_NO_PEERS;
    }

    if (depth == 0) {
      depth = 1;
    } else if (depth > 100) {
      depth = 100;
    }

    OUTCOME_TRY(blocksync_->makeRequest(
        last_peer_.value(), key.cids(), depth, blocksync::BLOCKS_AND_MESSAGES));

    global_wantlist_.insert(key.cids().begin(), key.cids().end());

    RequestCtx ctx(*this, key);

    tipset_requests_.insert({key.hash(), std::move(ctx)});
    return outcome::success();
  }

  TipsetLoader::RequestCtx::RequestCtx(TipsetLoader &o, const TipsetKey &key)
      : owner(o), tipset_key(key) {
    wantlist.insert(key.cids().begin(), key.cids().end());
    blocks_filled.resize(key.cids().size());
  }

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

    auto res = Tipset::create(tipset_key.hash(), std::move(blocks_filled));
    owner.onRequestCompleted(tipset_key.hash(), std::move(res));
  }

  void TipsetLoader::RequestCtx::onError(const CID &cid,
                                         std::error_code error) {
    if (is_bad_tipset) {
      return;
    }

    auto it = wantlist.find(cid);
    if (it == wantlist.end()) {
      // not our block
      return;
    }

    is_bad_tipset = true;

    owner.onRequestCompleted(tipset_key.hash(), error);
  }

  void TipsetLoader::onRequestCompleted(TipsetHash hash,
                                        outcome::result<TipsetCPtr> result) {
    tipset_requests_.erase(hash);

    events::TipsetStored event{.hash = std::move(hash),
                               .tipset = std::move(result),
                               .proceed_sync_from = boost::none};

    if (event.tipset) {
      auto bottom = chain_db_->storeTipset(event.tipset.value(),
                                           event.tipset.value()->getParents());
      if (bottom.has_error()) {
        event.tipset = bottom.error();
      } else {
        event.proceed_sync_from = std::move(bottom.value());
      }
    }

    events_->signalTipsetStored(std::move(event));
  }

  void TipsetLoader::onBlock(const events::BlockStored &event) {
    auto it = global_wantlist_.find(event.block_cid);
    if (it == global_wantlist_.end()) {
      // not our block
      return;
    }

    global_wantlist_.erase(event.block_cid);

    if (event.block) {
      const auto &header = event.block.value().header;
      for (auto &[_, ctx] : tipset_requests_) {
        ctx.onBlockSynced(event.block_cid, header);
      }
    } else {
      for (auto &[_, ctx] : tipset_requests_) {
        ctx.onError(event.block_cid, event.block.error());
      }
    }
  }

}  // namespace fc::sync

OUTCOME_CPP_DEFINE_CATEGORY(fc::sync, TipsetLoader::Error, e) {
  using E = fc::sync::TipsetLoader::Error;

  switch (e) {
    case E::TIPSET_LOADER_NOT_INITIALIZED:
      return "tipset loader: not initialized";
    case E::TIPSET_LOADER_NO_PEERS:
      return "tipset loader: no peers";
    case E::TIPSET_LOADER_BAD_TIPSET:
      return "tipset loader: bad tipset";
    default:
      break;
  }
  return "TipsetLoader::Error: unknown error";
}
