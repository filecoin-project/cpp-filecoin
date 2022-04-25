/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor_method.hpp"

namespace fc::vm::actor::builtin {

  // These methods must be actual with the last version of actors

  enum class MinerActor : MethodNumber {
    kConstruct = 1,
    kControlAddresses,
    kChangeWorkerAddress,
    kChangePeerId,
    kSubmitWindowedPoSt,
    kPreCommitSector,
    kProveCommitSector,
    kExtendSectorExpiration,
    kTerminateSectors,
    kDeclareFaults,
    kDeclareFaultsRecovered,
    kOnDeferredCronEvent,
    kCheckSectorProven,
    kApplyRewards,  // since v2, AddLockedFund for v0
    kReportConsensusFault,
    kWithdrawBalance,
    kConfirmSectorProofsValid,
    kChangeMultiaddresses,
    kCompactPartitions,
    kCompactSectorNumbers,
    kConfirmUpdateWorkerKey,  // since v2
    kRepayDebt,               // since v2
    kChangeOwnerAddress,      // since v2
    kDisputeWindowedPoSt,     // since v3
    kPreCommitBatch,          // since v5
    kProveCommitAggregate,    // since v5
    kProveReplicaUpdates,     // since v7
  }

}  // namespace fc::vm::actor::builtin
