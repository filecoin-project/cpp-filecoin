/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SHARED_HPP
#define CPP_FILECOIN_SHARED_HPP

#include "primitives/address/address.hpp"
#include "vm/actor/builtin/miner/miner_actor.hpp"
#include "vm/runtime/runtime.hpp"

/**
 * Code shared by multiple built-in actors
 */

namespace fc::vm::actor::builtin {

  using runtime::Runtime;

  /**
   * Get worker address
   * @param runtime
   * @param miner
   * @return
   */
  inline outcome::result<miner::ControlAddresses::Result> requestMinerControlAddress(
      Runtime &runtime, const Address &miner) {
    return runtime.sendM<miner::ControlAddresses>(miner, {}, 0);
  }
}  // namespace fc::vm::actor::builtin

#endif  // CPP_FILECOIN_SHARED_HPP
