/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_SUBCHAIN_LOADER_HPP
#define CPP_FILECOIN_SYNC_SUBCHAIN_LOADER_HPP

#include "fwd.hpp"

#include <libp2p/protocol/common/scheduler.hpp>

namespace fc::sync {

  namespace events {
    struct TipsetStored;
  }

  class TipsetLoader;
  class ChainDb;

  class SubchainLoader {
   public:
    struct Status {
      enum StatusCode {
        IDLE,
        IN_PROGRESS,
        SYNCED_TO_GENESIS,
        INTERRUPTED = -1,
        BAD_BLOCKS = -2,
        INTERNAL_ERROR = -3,
      };

      StatusCode code = IDLE;
      std::error_code error;
      boost::optional<PeerId> peer;
      boost::optional<TipsetKey> head;
      boost::optional<TipsetHash> last_loaded;
      boost::optional<TipsetHash> next;
      uint64_t total = 0;
    };

    using Callback = std::function<void(Status status)>;

    SubchainLoader(libp2p::protocol::Scheduler &scheduler,
                   TipsetLoader &tipset_loader,
                   ChainDb &chain_db,
                   Callback callback);

    void start(PeerId peer, TipsetKey head, uint64_t probable_depth);

    void cancel();

    bool isActive() const;

    const Status &getStatus() const;

    void onTipsetStored(const events::TipsetStored &e);

   private:
    void internalError(std::error_code e);

    void scheduleCallback();

    void nextTarget(boost::optional<TipsetCPtr> last_loaded);

    libp2p::protocol::Scheduler &scheduler_;
    TipsetLoader &tipset_loader_;
    ChainDb &chain_db_;
    bool active_ = false;
    Status status_;
    Callback callback_;
    libp2p::protocol::scheduler::Handle cb_handle_;
  };

}  // namespace fc::sync

#endif  // CPP_FILECOIN_SYNC_JOB_HPP
