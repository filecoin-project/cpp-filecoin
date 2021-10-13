/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v2/market/market_actor.hpp"

namespace fc::vm::actor::builtin::v3::market {

  // TODO (m.tagirov) Implement Market Actor v3

  using Construct = v2::market::Construct;
  using AddBalance = v2::market::AddBalance;
  using WithdrawBalance = v2::market::WithdrawBalance;
  using PublishStorageDeals = v2::market::PublishStorageDeals;
  using VerifyDealsForActivation = v2::market::VerifyDealsForActivation;
  using ActivateDeals = v2::market::ActivateDeals;
  using OnMinerSectorsTerminate = v2::market::OnMinerSectorsTerminate;
  using ComputeDataCommitment = v2::market::ComputeDataCommitment;
  using CronTick = v2::market::CronTick;

}  // namespace fc::vm::actor::builtin::v3::market
