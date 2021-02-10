/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_SYNC_JOB_HPP
#define CPP_FILECOIN_SYNC_SYNC_JOB_HPP

#include <queue>

#include "node/interpret_job.hpp"
#include "peers.hpp"
#include "storage/buffer_map.hpp"
#include "tipset_request.hpp"

namespace fc::sync {

  /// Active object which downloads and indexes tipsets. Keeps track of peers
  /// which are also nodes (to make requests to them)
  class SyncJob {
   public:
    SyncJob(std::shared_ptr<libp2p::Host> host,
            std::shared_ptr<libp2p::protocol::Scheduler> scheduler,
            std::shared_ptr<InterpretJob> interpret_job,
            TsBranches &ts_branches,
            TsBranchPtr ts_main,
            TsLoadPtr ts_load,
            IpldPtr ipld);

    /// Listens to PossibleHead and PeerConnected events
    void start(std::shared_ptr<events::Events> events);

   private:
    struct DownloadTarget {
      TipsetKey head;
      uint64_t height;

      // peers reported this target
      std::unordered_set<PeerId> peers;

      boost::optional<PeerId> active_peer;
    };

    using DownloadTargets =
        std::map<std::pair<Height, TipsetHash>, DownloadTarget>;

    void onPossibleHead(const events::PossibleHead &e);

    TipsetCPtr getLocal(const TipsetKey &tsk);

    void onTs(const boost::optional<PeerId> &peer, TipsetCPtr ts);

    void fetch(const PeerId &peer, const TipsetKey &tsk);

    void fetchDequeue();

    void downloaderCallback(TipsetRequest::Result r);

    std::shared_ptr<libp2p::Host> host_;
    std::shared_ptr<libp2p::protocol::Scheduler> scheduler_;
    std::shared_ptr<InterpretJob> interpret_job_;
    TsBranches &ts_branches_;
    TsBranchPtr ts_main_;
    TsLoadPtr ts_load_;
    IpldPtr ipld_;
    std::mutex branches_mutex_;

    std::queue<std::pair<PeerId, TipsetKey>> requests_;
    std::mutex requests_mutex_;

    std::shared_ptr<events::Events> events_;
    Peers peers_;

    events::Connection head_interpreted_event_;
    events::Connection possible_head_event_;

    std::shared_ptr<TipsetRequest> request_;
  };

}  // namespace fc::sync

#endif  // CPP_FILECOIN_SYNC_SYNC_JOB_HPP
