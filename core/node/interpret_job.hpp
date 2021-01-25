/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_INTERPRET_JOB_HPP
#define CPP_FILECOIN_SYNC_INTERPRET_JOB_HPP

#include "events_fwd.hpp"

#include "storage/buffer_map.hpp"

namespace fc::sync {
  using vm::interpreter::CachedInterpreter;

  class ChainDb;

  /// Active objects which interprets parts of chain which are downloaded but
  /// not yet interpreted
  class InterpretJob {
   public:
    static std::shared_ptr<InterpretJob> create(
        std::shared_ptr<CachedInterpreter> interpreter,
        std::shared_ptr<libp2p::protocol::Scheduler> scheduler,
        std::shared_ptr<ChainDb> chain_db,
        IpldPtr ipld,
        std::shared_ptr<blockchain::weight::WeightCalculator>
            weight_calculator);

    virtual ~InterpretJob() = default;

    /// Listens to HeadDownloaded events, interprets subchains not yet
    /// interpreted, emits HeadInterpreted events
    virtual void start(std::shared_ptr<events::Events> events) = 0;
  };

}  // namespace fc::sync

#endif  // CPP_FILECOIN_SYNC_INTERPRET_JOB_HPP
