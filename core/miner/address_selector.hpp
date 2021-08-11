/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/full_node/node_api.hpp"
#include "common/outcome.hpp"
#include "primitives/address/address.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"

namespace fc::mining {
  using api::FullNodeApi;
  using api::MinerInfo;
  using primitives::TokenAmount;
  using primitives::address::Address;

  /**
   * SelectAddress takes the maximal possible transaction fee from configs and
   * chooses one of control addresses with minimal balance that is more than good
   * funds to make miner work as long as possible. If no suitble control address
   * were found, function returns worker address.
   */
  inline outcome::result<Address> SelectAddress(
      const MinerInfo &miner_info,
      const TokenAmount &good_funds,
      const std::shared_ptr<FullNodeApi> &api) {
    TokenAmount finder_balance;
    auto finder = miner_info.control.end();
    for (auto address = miner_info.control.begin();
         address != miner_info.control.end();
         ++address) {
      OUTCOME_TRY(address_balance, api->WalletBalance(*address));
      if (address_balance >= good_funds
          && (finder == miner_info.control.end()
              || finder_balance > address_balance)) {
        finder = address;
        finder_balance = address_balance;
      }
    }
    if (finder == miner_info.control.end()) {
      return miner_info.worker;
    }
    return *finder;
  }

}  // namespace fc::mining
