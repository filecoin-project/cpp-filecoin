/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <queue>

#include "blocksync_request.hpp"
#include "common/io_thread.hpp"
#include "primitives/tipset/chain.hpp"
#include "storage/buffer_map.hpp"
#include "vm/interpreter/interpreter.hpp"

namespace fc::sync {
  using blocksync::BlocksyncRequest;
  using primitives::tipset::PutBlockHeader;
  using primitives::tipset::chain::KvPtr;
  using vm::interpreter::Interpreter;
  using vm::interpreter::InterpreterCache;
  using InterpreterResult = vm::interpreter::Result;

  class SyncJob {
   public:
    SyncJob(std::shared_ptr<libp2p::Host> host,
            std::shared_ptr<ChainStoreImpl> chain_store,
            std::shared_ptr<libp2p::protocol::Scheduler> scheduler,
            std::shared_ptr<Interpreter> interpreter,
            std::shared_ptr<InterpreterCache> interpreter_cache,
            SharedMutexPtr ts_branches_mutex,
            TsBranchesPtr ts_branches,
            KvPtr ts_main_kv,
            TsBranchPtr ts_main,
            TsLoadPtr ts_load,
            std::shared_ptr<PutBlockHeader> put_block_header,
            IpldPtr ipld);

    /// Listens to PossibleHead and PeerConnected events
    void start(std::shared_ptr<events::Events> events);

    uint64_t metricAttachedHeight() const;

   private:
    void onPossibleHead(const events::PossibleHead &e);

    TipsetCPtr getLocal(const TipsetCPtr &ts);
    TipsetCPtr getLocal(const TipsetKey &tsk);

    void compactBranches();

    void onTs(const boost::optional<PeerId> &peer, TipsetCPtr ts);

    void attach(TsBranchPtr branch);

    void updateTarget(TsBranchPtr last);

    void onInterpret(TipsetCPtr ts, const InterpreterResult &result);

    bool checkParent(TipsetCPtr ts);

    void interpretDequeue();

    void fetch(const PeerId &peer, const TipsetKey &tsk);

    void fetchDequeue();

    void downloaderCallback(BlocksyncRequest::Result r);

    std::shared_ptr<libp2p::Host> host_;
    std::shared_ptr<ChainStoreImpl> chain_store_;
    std::shared_ptr<libp2p::protocol::Scheduler> scheduler_;
    std::shared_ptr<Interpreter> interpreter_;
    std::shared_ptr<InterpreterCache> interpreter_cache_;
    SharedMutexPtr ts_branches_mutex_;
    TsBranchesPtr ts_branches_;
    KvPtr ts_main_kv_;
    TsBranchPtr ts_main_;
    TsLoadPtr ts_load_;
    std::shared_ptr<PutBlockHeader> put_block_header_;
    IpldPtr ipld_;
    TsBranches attached_;
    std::pair<TsBranchPtr, BigInt> attached_heaviest_;
    TipsetCPtr interpret_ts_;
    bool interpreting_{false};
    IoThread thread;
    IoThread interpret_thread;

    std::queue<std::pair<PeerId, TipsetKey>> requests_;
    std::mutex requests_mutex_;

    std::queue<TipsetCPtr> interpret_queue_;

    std::shared_ptr<events::Events> events_;

    events::Connection message_event_;
    events::Connection block_event_;
    events::Connection possible_head_event_;

    std::shared_ptr<BlocksyncRequest> request_;

    using Clock = std::chrono::steady_clock;
    Clock::time_point request_expiry_;
  };

}  // namespace fc::sync
