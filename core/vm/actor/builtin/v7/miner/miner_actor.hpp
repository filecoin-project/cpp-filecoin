/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v5/miner/miner_actor.hpp"

namespace fc::vm::actor::builtin::v7::miner {
  using primitives::DealId;
  using primitives::RleBitset;
  using primitives::SectorNumber;
  using primitives::sector::RegisteredUpdateProof;

  // TODO(m.tagirov): implement
  using Construct = v5::miner::Construct;
  using ControlAddresses = v5::miner::ControlAddresses;
  using ChangeWorkerAddress = v5::miner::ChangeWorkerAddress;
  using ChangePeerId = v5::miner::ChangePeerId;
  using SubmitWindowedPoSt = v5::miner::SubmitWindowedPoSt;
  using PreCommitSector = v5::miner::PreCommitSector;
  using ProveCommitSector = v5::miner::ProveCommitSector;
  using ExtendSectorExpiration = v5::miner::ExtendSectorExpiration;
  using TerminateSectors = v5::miner::TerminateSectors;
  using DeclareFaults = v5::miner::DeclareFaults;
  using DeclareFaultsRecovered = v5::miner::DeclareFaultsRecovered;
  using OnDeferredCronEvent = v5::miner::OnDeferredCronEvent;
  using CheckSectorProven = v5::miner::CheckSectorProven;
  using ApplyRewards = v5::miner::ApplyRewards;
  using ReportConsensusFault = v5::miner::ReportConsensusFault;
  using WithdrawBalance = v5::miner::WithdrawBalance;
  using ConfirmSectorProofsValid = v5::miner::ConfirmSectorProofsValid;
  using ChangeMultiaddresses = v5::miner::ChangeMultiaddresses;
  using CompactPartitions = v5::miner::CompactPartitions;
  using CompactSectorNumbers = v5::miner::CompactSectorNumbers;
  using ConfirmUpdateWorkerKey = v5::miner::ConfirmUpdateWorkerKey;
  using RepayDebt = v5::miner::RepayDebt;
  using ChangeOwnerAddress = v5::miner::ChangeOwnerAddress;
  using DisputeWindowedPoSt = v5::miner::DisputeWindowedPoSt;

  struct ReplicaUpdate {
    SectorNumber sector{};
    uint64_t deadline{};
    uint64_t partition{};
    CID comm_r;
    std::vector<DealId> deals;
    RegisteredUpdateProof update_type{};
    Bytes proof;
  };
  CBOR_TUPLE(ReplicaUpdate,
             sector,
             deadline,
             partition,
             comm_r,
             deals,
             update_type,
             proof)

  struct ProveReplicaUpdates : ActorMethodBase<27> {
    struct Params {
      std::vector<ReplicaUpdate> updates;
    };
    using Result = RleBitset;
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(ProveReplicaUpdates::Params, updates)
}  // namespace fc::vm::actor::builtin::v7::miner
