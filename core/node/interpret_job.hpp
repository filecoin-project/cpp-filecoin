/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_INTERPRET_JOB_HPP
#define CPP_FILECOIN_SYNC_INTERPRET_JOB_HPP

#include <libp2p/protocol/common/scheduler.hpp>

#include "chain_db.hpp"
#include "events.hpp"
#include "storage/buffer_map.hpp"
#include "vm/interpreter/interpreter.hpp"

namespace fc::sync {

  class InterpretJob {
   public:
    struct Status {
      Height current_height = 0;
      Height target_height = 0;
      bool active = false;
    };

    using Result = events::HeadInterpreted;
    using Callback = std::function<void(Result result)>;

    InterpretJob(std::shared_ptr<storage::PersistentBufferMap> kv_store,
                 std::shared_ptr<vm::interpreter::Interpreter> interpreter,
                 libp2p::protocol::Scheduler &scheduler,
                 ChainDb &chain_db,
                 IpfsStoragePtr ipld,
                 Callback callback);

    /// Starts async head interpret job, but returns immediate result if
    /// the head is already interpreted
    outcome::result<boost::optional<Result>> start(
        const TipsetKey &head);

    // returns last status and clears all
    Status cancel();

    const Status &getStatus() const;

   private:
    void done();

    void scheduleStep();

    void nextStep();

    TipsetCPtr getNextTipset();

    std::shared_ptr<storage::PersistentBufferMap> kv_store_;
    std::shared_ptr<vm::interpreter::Interpreter> interpreter_;
    libp2p::protocol::Scheduler &scheduler_;
    ChainDb &chain_db_;
    IpfsStoragePtr ipld_;
    Callback callback_;

    Status status_;
    events::HeadInterpreted result_;
    TipsetCPtr target_head_;
    std::vector<TipsetCPtr> next_steps_;
    size_t step_cursor_ = 0;
    libp2p::protocol::scheduler::Handle cb_handle_;
  };

}  // namespace fc::sync

#endif  // CPP_FILECOIN_SYNC_INTERPRET_JOB_HPP
