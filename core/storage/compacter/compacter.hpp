/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/io_thread.hpp"
#include "primitives/tipset/chain.hpp"
#include "storage/compacter/queue.hpp"
#include "storage/ipld/cids_ipld.hpp"
#include "storage/leveldb/prefix.hpp"
#include "vm/interpreter/interpreter.hpp"

namespace fc::vm::interpreter {
  using primitives::tipset::TipsetCPtr;

  struct CompacterInterpreter : Interpreter {
    outcome::result<Result> interpret(TsBranchPtr ts_branch,
                                      const TipsetCPtr &tipset) const override;

    std::shared_ptr<Interpreter> interpreter;
    SharedMutexPtr mutex;
  };
}  // namespace fc::vm::interpreter

namespace fc::storage::compacter {
  using ipld::CidsIpld;
  using primitives::tipset::TipsetCPtr;
  using vm::interpreter::CompacterInterpreter;
  using vm::interpreter::Interpreter;
  using vm::interpreter::InterpreterCache;

  struct CompacterIpld;

  struct CompacterPutBlockHeader : primitives::tipset::PutBlockHeader {
    std::weak_ptr<CompacterIpld> _compacter;

    void put(const CbCid &key, BytesIn value) override;
  };

  struct CompacterIpld : CbIpld, std::enable_shared_from_this<CompacterIpld> {
    using CbIpld::get, CbIpld::put;

    bool get(const CbCid &key, Buffer *value) const override;
    void put(const CbCid &key, BytesIn value) override;

    void open();
    bool asyncStart();
    void doStart();
    void resume();
    void flow();
    void headersBatch();
    void queueLoop();
    void finish();
    void pushState(const CbCid &state);
    void lookbackState(const CbCid &state);
    void copy(const CbCid &key);

    uint64_t compact_on_car{};
    size_t epochs_full_state{};
    size_t epochs_lookback_state{};
    size_t epochs_messages{};

    std::string path;
    IoThread thread;
    std::shared_ptr<CidsIpld> old_ipld;
    SharedMutexPtr ts_mutex;
    TsBranchesPtr ts_branches;
    std::shared_ptr<InterpreterCache> interpreter_cache;
    TsLoadPtr ts_load;
    OneKey start_head_key{"", {}};
    TipsetCPtr start_head;
    OneKey headers_top_key{"", {}};
    TipsetCPtr headers_top;
    TipsetCPtr state_bottom;
    std::shared_ptr<CompacterQueue> queue;
    std::shared_ptr<CompacterInterpreter> interpreter;
    std::shared_ptr<CompacterPutBlockHeader> put_block_header;
    TsBranchPtr ts_main;
    mutable std::shared_mutex ipld_mutex;
    std::shared_ptr<CidsIpld> new_ipld;
    std::atomic_bool flag;
    bool use_new_ipld{};
  };
}  // namespace fc::storage::compacter
