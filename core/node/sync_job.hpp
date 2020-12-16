/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_SYNC_JOB_HPP
#define CPP_FILECOIN_SYNC_SYNC_JOB_HPP

#include "peers.hpp"
#include "storage/buffer_map.hpp"
#include "tipset_request.hpp"

namespace fc::sync {

  class SyncJob {
   public:
    SyncJob(std::shared_ptr<ChainDb> chain_db,
            std::shared_ptr<libp2p::Host> host,
            std::shared_ptr<libp2p::protocol::Scheduler> scheduler,
            IpldPtr ipld);

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

    bool adjustTarget(DownloadTarget& target);

    void enqueue(DownloadTarget target);

    void newJob(DownloadTarget target, bool make_deep_request);

    void downloaderCallback(TipsetRequest::Result r);

    boost::optional<DownloadTarget> dequeue();

    std::shared_ptr<ChainDb> chain_db_;
    std::shared_ptr<libp2p::Host> host_;
    std::shared_ptr<libp2p::protocol::Scheduler> scheduler_;
    IpldPtr ipld_;

    std::shared_ptr<events::Events> events_;
    Peers peers_;
    DownloadTargets pending_targets_;

    /// Last known height needed to limit the depth of sync queries if possible
    Height last_known_height_ = 0;

    events::Connection possible_head_event_;
    events::Connection peer_connected_event_;

    boost::optional<TipsetRequest> request_;
    boost::optional<DownloadTarget> now_active_;
  };

}  // namespace fc::sync

#endif  // CPP_FILECOIN_SYNC_SYNC_JOB_HPP
