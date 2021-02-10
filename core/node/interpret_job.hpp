/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_INTERPRET_JOB_HPP
#define CPP_FILECOIN_SYNC_INTERPRET_JOB_HPP

#include "events_fwd.hpp"

namespace fc::sync {
  using primitives::tipset::TipsetCPtr;
  using vm::interpreter::CachedInterpreter;

  /// Active objects which interprets parts of chain which are downloaded but
  /// not yet interpreted
  class InterpretJob {
   public:
    static std::shared_ptr<InterpretJob> create(
        std::shared_ptr<CachedInterpreter> interpreter,
        const TsBranches &ts_branches,
        IpldPtr ipld,
        std::shared_ptr<events::Events> events);

    virtual ~InterpretJob() = default;
    virtual void add(TipsetCPtr ts) = 0;
  };

}  // namespace fc::sync

#endif  // CPP_FILECOIN_SYNC_INTERPRET_JOB_HPP
