/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v2/miner/miner_actor.hpp"

namespace fc::vm::actor::builtin::v2::miner {

  ACTOR_METHOD_IMPL(ApplyRewards) {
    // TODO (a.chernyshov) implement
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(ConfirmUpdateWorkerKey) {
    // TODO (a.chernyshov) implement
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(RepayDebt) {
    // TODO (a.chernyshov) implement
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(ChangeOwnerAddress) {
    // TODO (a.chernyshov) implement
    return outcome::success();
  }

  const ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<ControlAddresses>(),
      exportMethod<ChangeWorkerAddress>(),
      exportMethod<ChangePeerId>(),
      exportMethod<SubmitWindowedPoSt>(),
      exportMethod<PreCommitSector>(),
      exportMethod<ProveCommitSector>(),
      exportMethod<ExtendSectorExpiration>(),
      exportMethod<TerminateSectors>(),
      exportMethod<DeclareFaults>(),
      exportMethod<DeclareFaultsRecovered>(),
      exportMethod<OnDeferredCronEvent>(),
      exportMethod<CheckSectorProven>(),
      exportMethod<ApplyRewards>(),
      exportMethod<ReportConsensusFault>(),
      exportMethod<WithdrawBalance>(),
      exportMethod<ConfirmSectorProofsValid>(),
      exportMethod<ChangeMultiaddresses>(),
      exportMethod<CompactPartitions>(),
      exportMethod<CompactSectorNumbers>(),
      exportMethod<ConfirmUpdateWorkerKey>(),
      exportMethod<RepayDebt>(),
      exportMethod<ChangeOwnerAddress>(),
  };

}  // namespace fc::vm::actor::builtin::v2::miner
