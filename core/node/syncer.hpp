/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_SYNCER_HPP
#define CPP_FILECOIN_SYNC_SYNCER_HPP

#include "interpret_job.hpp"
#include "peers.hpp"
#include "subchain_loader.hpp"

namespace fc::sync {

  class TipsetLoader;

  class Syncer {
   public:
    Syncer(std::shared_ptr<libp2p::protocol::Scheduler> scheduler,
           std::shared_ptr<TipsetLoader> tipset_loader,
           std::shared_ptr<ChainDb> chain_db,
           std::shared_ptr<storage::PersistentBufferMap> kv_store,
           std::shared_ptr<vm::interpreter::Interpreter> interpreter,
           IpfsStoragePtr ipld);

    void start(std::shared_ptr<events::Events> events);

   private:
    void downloaderCallback(SubchainLoader::Status status);

    void interpreterCallback(InterpretJob::Result result);

    void newInterpretJob(TipsetKey key);

    void onPossibleHead(const events::PossibleHead &e);

    boost::optional<PeerId> choosePeer(boost::optional<PeerId> candidate);

    void newDownloadJob(PeerId peer,
                        TipsetKey head,
                        Height height,
                        bool make_deep_request);

    struct DownloadTarget {
      TipsetKey head_tipset;
      uint64_t height;
    };

    using DownloadTargets = std::unordered_map<PeerId, DownloadTarget>;
    using InterpretTargets = std::deque<TipsetKey>;

    std::shared_ptr<libp2p::protocol::Scheduler> scheduler_;
    std::shared_ptr<TipsetLoader> tipset_loader_;
    std::shared_ptr<ChainDb> chain_db_;

    /// One download job at the moment, they could be parallel
    SubchainLoader downloader_;

    /// Interpreter job, no need to parallel them, they may intersect
    InterpretJob interpreter_;

    Peers peers_;

    DownloadTargets pending_targets_;
    InterpretTargets pending_interpret_targets_;

    /// Last known height needed to limit the depth of sync queries if possible
    Height last_known_height_ = 0;

    std::shared_ptr<events::Events> events_;
    events::Connection possible_head_event_;
    events::Connection tipset_stored_event_;
    events::Connection peer_disconnected_event_;
  };

}  // namespace fc::sync

#endif  // CPP_FILECOIN_SYNC_SYNCER_HPP
