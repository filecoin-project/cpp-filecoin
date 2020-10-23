/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_INTERPRET_JOB_HPP
#define CPP_FILECOIN_SYNC_INTERPRET_JOB_HPP

#include <libp2p/protocol/common/scheduler.hpp>
#include "vm/interpreter/interpreter.hpp"
#include "storage/buffer_map.hpp"

#include "chain_db.hpp"

namespace fc::sync {

  class InterpreterJob : public std::enable_shared_from_this<InterpreterJob> {
   public:
    struct Result {
      TipsetCPtr last_interpreted;
      outcome::result<vm::interpreter::Result> result;
    };

    struct Status {
      Height current_height = 0;
      Height target_height = 0;
    };

    using Callback = std::function<void(const Result &result)>;

    InterpreterJob(std::shared_ptr<storage::PersistentBufferMap> kv_store,
                   std::shared_ptr<vm::interpreter::Interpreter> interpreter,
                   libp2p::protocol::Scheduler &scheduler,
                   ChainDb &chain_db,
                   IpfsStoragePtr ipld,
                   Callback callback);

    outcome::result<void> start(const TipsetKey &head);

    // returns last status and clears all
    Status cancel();

    const Status &getStatus() const;

    //const Result& getResult() const;

   private:
    void scheduleResult();

    void scheduleStep();

    void nextStep();

    TipsetCPtr getNextTipset();

    std::shared_ptr<storage::PersistentBufferMap> kv_store_;
    std::shared_ptr<vm::interpreter::Interpreter> interpreter_;
    libp2p::protocol::Scheduler &scheduler_;
    ChainDb &chain_db_;
    IpfsStoragePtr ipld_;
    Callback callback_;

    bool active_ = false;
    Status status_;
    Result result_;
    TipsetCPtr target_head_;
    std::vector<TipsetCPtr> next_steps_;
    size_t step_cursor_ = 0;
    libp2p::protocol::scheduler::Handle cb_handle_;
  };

}  // namespace fc::sync

#endif  // CPP_FILECOIN_SYNC_INTERPRET_JOB_HPP
