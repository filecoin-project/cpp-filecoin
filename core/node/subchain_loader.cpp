/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/logger.hpp"
#include "subchain_loader.hpp"
#include "chain_db.hpp"
#include "tipset_loader.hpp"
#include "events.hpp"

namespace fc::sync {

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("subchain_loader");
      return logger.get();
    }
  }  // namespace

  SubchainLoader::SubchainLoader(libp2p::protocol::Scheduler &scheduler,
                                 TipsetLoader &tipset_loader,
                                 ChainDb &chain_db,
                                 Callback callback)
      : scheduler_(scheduler),
        tipset_loader_(tipset_loader),
        chain_db_(chain_db),
        callback_(std::move(callback)) {
    assert(callback_);
  }

  void SubchainLoader::start(PeerId peer,
                             TipsetKey head,
                             uint64_t probable_depth) {
    if (active_) {
      log()->error("current job is still active, ignoring the new one");
      return;
    }
    active_ = true;

    status_.peer = std::move(peer);
    status_.head = std::move(head);

    try {
      if (!chain_db_.tipsetIsStored(head.hash())) {
        // not indexed, loading...
        OUTCOME_EXCEPT(tipset_loader_.loadTipsetAsync(
            status_.head.value(), status_.peer, probable_depth));
        status_.next = status_.head.value().hash();
        status_.code = Status::IN_PROGRESS;
        return;
      }

      OUTCOME_EXCEPT(maybe_next_target, chain_db_.getUnsyncedBottom(head));

      nextTarget(std::move(maybe_next_target));

    } catch (const std::system_error &e) {
      internalError(e.code());
    }
  }

  void SubchainLoader::cancel() {
    if (active_) {
      Status s;
      std::swap(s, status_);
      cb_handle_.cancel();
      active_ = false;
    }
  }

  bool SubchainLoader::isActive() const {
    return active_;
  }

  const SubchainLoader::Status &SubchainLoader::getStatus() const {
    return status_;
  }

  void SubchainLoader::onTipsetStored(const events::TipsetStored &e) {
    if (status_.code != Status::IN_PROGRESS || !status_.next.has_value()
        || e.hash != status_.next.value()) {
      // dont need this tipset
      return;
    }

    try {
      OUTCOME_EXCEPT(e.tipset);
      nextTarget(std::move(e.proceed_sync_from));

    } catch (const std::system_error &e) {
      // TODO (artem) separate bad blocks error vs. other errors
      internalError(e.code());
    }
  }

  void SubchainLoader::internalError(std::error_code e) {
    log()->error("internal error, {}", e.message());
    status_.error = e;
    status_.code = Status::INTERNAL_ERROR;
    scheduleCallback();
  }

  void SubchainLoader::scheduleCallback() {
    cb_handle_ = scheduler_.schedule([this]() {
      Status s;
      std::swap(s, status_);
      active_ = false;
      callback_(s);
    });
  }

  void SubchainLoader::nextTarget(boost::optional<TipsetCPtr> last_loaded) {
    if (!last_loaded) {
      status_.next = TipsetHash{};
      status_.code = Status::SYNCED_TO_GENESIS;
      scheduleCallback();
      return;
    }

    auto &roots = last_loaded.value();

    status_.last_loaded = roots->key.hash();
    auto next_key = roots->getParents();
    status_.next = next_key.hash();

    OUTCOME_EXCEPT(tipset_loader_.loadTipsetAsync(
        std::move(next_key), status_.peer, roots->height() - 1));
  }

}  // namespace fc::sync
