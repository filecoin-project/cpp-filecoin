/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/basic/scheduler.hpp>
#include <queue>

#include "common/io_thread.hpp"
#include "node/blocksync_request.hpp"
#include "primitives/tipset/chain.hpp"
#include "storage/buffer_map.hpp"
#include "vm/interpreter/interpreter.hpp"

namespace fc::sync::peer_height {
  struct PeerHeight;
}  // namespace fc::sync::peer_height

namespace fc::sync::fetch_msg {
  struct FetchMsg;
}  // namespace fc::sync::fetch_msg

namespace fc::sync {
  using blocksync::BlocksyncRequest;
  using fetch_msg::FetchMsg;
  using libp2p::basic::Scheduler;
  using peer_height::PeerHeight;
  using primitives::tipset::PutBlockHeader;
  using vm::interpreter::Interpreter;
  using vm::interpreter::InterpreterCache;
  using InterpreterResult = vm::interpreter::Result;

  class SyncJob {
   public:
    SyncJob(std::shared_ptr<libp2p::Host> host,
            std::shared_ptr<boost::asio::io_context> io,
            std::shared_ptr<ChainStoreImpl> chain_store,
            std::shared_ptr<Scheduler> scheduler,
            std::shared_ptr<Interpreter> interpreter,
            std::shared_ptr<InterpreterCache> interpreter_cache,
            SharedMutexPtr ts_branches_mutex,
            TsBranchesPtr ts_branches,
            TsBranchPtr ts_main,
            TsLoadPtr ts_load,
            std::shared_ptr<PutBlockHeader> put_block_header,
            IpldPtr ipld);

    /// Listens to PossibleHead and PeerConnected events
    void start(std::shared_ptr<events::Events> events);

    ChainEpoch metricAttachedHeight() const;

   private:
    void onPossibleHead(const events::PossibleHead &e);

    TipsetCPtr getLocal(const TipsetKey &tsk);

    void compactBranches();

    void onTs(const boost::optional<PeerId> &peer, TipsetCPtr ts);

    void attach(TsBranchPtr branch);

    void updateTarget(const TsBranchPtr &last);

    void onInterpret(const TipsetCPtr &ts, const InterpreterResult &result);

    bool checkParent(const TipsetCPtr &ts);

    void interpretDequeue();

    void fetch(const PeerId &peer, const TipsetKey &tsk);

    void fetchDequeue();

    void downloaderCallback(BlocksyncRequest::Result r);

    std::shared_ptr<libp2p::Host> host_;
    std::shared_ptr<boost::asio::io_context> io_;
    std::shared_ptr<ChainStoreImpl> chain_store_;
    std::shared_ptr<Scheduler> scheduler_;
    std::shared_ptr<Interpreter> interpreter_;
    std::shared_ptr<InterpreterCache> interpreter_cache_;
    SharedMutexPtr ts_branches_mutex_;
    TsBranchesPtr ts_branches_;
    TsBranchPtr ts_main_;
    TsLoadPtr ts_load_;
    std::shared_ptr<PutBlockHeader> put_block_header_;
    IpldPtr ipld_;
    TsBranches attached_;
    std::pair<TsBranchPtr, BigInt> attached_heaviest_;
    /** Tipset being interpreted at the moment. */
    TipsetCPtr interpret_ts_;
    bool interpreting_{false};
    IoThread interpret_thread;

    // TODO(turuslan): FIL-420 check cache memory usage
    std::queue<std::pair<PeerId, TipsetKey>> requests_;
    std::mutex requests_mutex_;

    std::shared_ptr<events::Events> events_;

    events::Connection message_event_;
    events::Connection block_event_;
    events::Connection possible_head_event_;

    std::shared_ptr<BlocksyncRequest> request_;

    using Clock = std::chrono::steady_clock;
    Clock::time_point request_expiry_;

    std::shared_ptr<PeerHeight> peers_;
    std::shared_ptr<FetchMsg> fetch_msg_;
  };

}  // namespace fc::sync
