/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/api.hpp"
#include "sector_storage/spec_interfaces/prover.hpp"
#include "vm/actor/builtin/miner/miner_actor.hpp"

namespace fc {
  using api::Address;
  using api::Api;
  using api::DeadlineInfo;
  using api::Tipset;
  using sector_storage::Prover;

  // synchronous WindowPoSt
  struct WindowPoStScheduler
      : public std::enable_shared_from_this<WindowPoStScheduler> {
    static constexpr auto kStartConfidence{4};

    outcome::result<std::shared_ptr<WindowPoStScheduler>> create(
        std::shared_ptr<Api> api,
        std::shared_ptr<Prover> prover,
        const Address &miner);
    outcome::result<void> onTipset(const Tipset &ts);

    std::shared_ptr<api::Channel<std::vector<api::HeadChange>>> channel;
    std::shared_ptr<Api> api;
    std::shared_ptr<Prover> prover;
    Address miner, worker;
    boost::optional<DeadlineInfo> last_deadline;
    uint64_t part_size;
  };
}  // namespace fc
