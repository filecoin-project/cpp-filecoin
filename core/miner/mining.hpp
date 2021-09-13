/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/functional/hash.hpp>
#include <libp2p/basic/scheduler.hpp>
#include <unordered_set>

#include "api/full_node/node_api.hpp"
#include "clock/utc_clock.hpp"
#include "sector_storage/spec_interfaces/prover.hpp"

namespace fc::mining {
  using api::Address;
  using api::BlockTemplate;
  using api::ChainEpoch;
  using api::FullNodeApi;
  using api::MiningBaseInfo;
  using api::SignedMessage;
  using clock::UTCClock;
  using libp2p::basic::Scheduler;
  using primitives::BigInt;
  using primitives::tipset::Tipset;
  using primitives::tipset::TipsetKey;
  using sector_storage::Prover;

  struct pair_hash {
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2> &pair) const {
      return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
    }
  };

  struct Mining : std::enable_shared_from_this<Mining> {
    static outcome::result<std::shared_ptr<Mining>> create(
        std::shared_ptr<Scheduler> scheduler,
        std::shared_ptr<UTCClock> clock,
        std::shared_ptr<FullNodeApi> api,
        std::shared_ptr<Prover> prover,
        const Address &miner);
    void start();
    void waitParent();
    outcome::result<void> waitBeacon();
    outcome::result<void> waitInfo();
    outcome::result<void> prepare();
    outcome::result<void> submit(BlockTemplate block1);
    outcome::result<void> bestParent();
    ChainEpoch height() const;
    void wait(int64_t sec, bool abs, Scheduler::Callback cb);
    outcome::result<boost::optional<BlockTemplate>> prepareBlock();

    std::shared_ptr<Scheduler> scheduler;
    std::shared_ptr<UTCClock> clock;
    std::shared_ptr<FullNodeApi> api;
    std::shared_ptr<Prover> prover;
    Address miner;
    uint64_t block_delay, propagation;
    boost::optional<Tipset> ts;
    BigInt weight;
    size_t skip{};
    std::unordered_set<std::pair<TipsetKey, size_t>, pair_hash> mined;
    boost::optional<MiningBaseInfo> info;
  };

  int64_t computeWinCount(BytesIn ticket,
                          const BigInt &power,
                          const BigInt &total_power);

  double ticketQuality(BytesIn ticket);
}  // namespace fc::mining
