/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v3/miner/miner_actor_utils.hpp"

#include "vm/actor/builtin/v3/account/account_actor.hpp"
#include "vm/actor/builtin/v3/storage_power/storage_power_actor.hpp"

namespace fc::vm::actor::builtin::v3::miner {

  outcome::result<void> MinerUtils::canPreCommitSealProof(
      RegisteredSealProof seal_proof_type,
      NetworkVersion network_version) const {
    // Do nothing for v3
    return outcome::success();
  }

  outcome::result<Address> MinerUtils::getPubkeyAddressFromAccountActor(
      const Address &address) const {
    return getRuntime().sendM<account::PubkeyAddress>(address, {}, 0);
  }

  outcome::result<void> MinerUtils::callPowerEnrollCronEvent(
      ChainEpoch event_epoch, const Buffer &params) const {
    OUTCOME_TRY(getRuntime().sendM<storage_power::EnrollCronEvent>(
        kStoragePowerAddress, {event_epoch, params}, 0));
    return outcome::success();
  }

}  // namespace fc::vm::actor::builtin::v3::miner
