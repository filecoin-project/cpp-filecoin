/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/common/metrics/instance_count.hpp>

#include "cbor_blake/ipld.hpp"
#include "fwd.hpp"
#include "primitives/tipset/tipset.hpp"
#include "storage/buffer_map.hpp"
#include "storage/ipfs/datastore.hpp"

namespace fc::vm::interpreter {
  using primitives::BigInt;
  using primitives::tipset::Height;
  using storage::PersistentBufferMap;

  enum class InterpreterError {
    kDuplicateMiner = 1,
    kTipsetMarkedBad,
    kChainInconsistency,
    kNotCached,
  };

  /** Tipset invocation result */
  struct Result {
    CID state_root;
    CID message_receipts;
    BigInt weight;
  };
  CBOR_TUPLE(Result, state_root, message_receipts, weight)

  struct InterpreterCache {
    struct Key {
      Key(const TipsetKey &tsk);
      Buffer key;

      LIBP2P_METRICS_INSTANCE_COUNT(fc::vm::interpreter::InterpreterCache::Key);
    };
    InterpreterCache(std::shared_ptr<PersistentBufferMap> kv,
                     std::shared_ptr<CbIpld> ipld);

    /**
     * Return tipset if it is present in cache
     * @param key - tipset hash
     * @return tipset invocation result if key is present or boost::none
     * otherwise
     */
    boost::optional<outcome::result<Result>> tryGet(const Key &key) const;
    outcome::result<Result> get(const Key &key) const;
    void set(const Key &key, const Result &result);

    /**
     * Marks that vm returned error for the tipset.
     * @param key
     */
    void markBad(const Key &key);
    void remove(const Key &key);

   private:
    std::shared_ptr<PersistentBufferMap> kv;
    std::shared_ptr<CbIpld> ipld_;

    LIBP2P_METRICS_INSTANCE_COUNT(fc::vm::interpreter::InterpreterCache);
  };

  class Interpreter {
   protected:
    using TipsetCPtr = primitives::tipset::TipsetCPtr;

   public:
    virtual ~Interpreter() = default;

    virtual outcome::result<Result> interpret(
        TsBranchPtr ts_branch, const TipsetCPtr &tipset) const = 0;
  };
}  // namespace fc::vm::interpreter

OUTCOME_HPP_DECLARE_ERROR(fc::vm::interpreter, InterpreterError);
