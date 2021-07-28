/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v3/miner/miner_actor.hpp"

namespace fc::vm::actor::builtin::v4::miner {

  // TODO implement
  using Construct = v3::miner::Construct;
  using ControlAddresses = v3::miner::ControlAddresses;
  using ChangeWorkerAddress = v3::miner::ChangeWorkerAddress;
  using ChangePeerId = v3::miner::ChangePeerId;
  using SubmitWindowedPoSt = v3::miner::SubmitWindowedPoSt;
  using PreCommitSector = v3::miner::PreCommitSector;
  using ProveCommitSector = v3::miner::ProveCommitSector;
  using ExtendSectorExpiration = v3::miner::ExtendSectorExpiration;
  using TerminateSectors = v3::miner::TerminateSectors;
  using DeclareFaults = v3::miner::DeclareFaults;
  using DeclareFaultsRecovered = v3::miner::DeclareFaultsRecovered;
  using OnDeferredCronEvent = v3::miner::OnDeferredCronEvent;
  using CheckSectorProven = v3::miner::CheckSectorProven;
  using ApplyRewards = v3::miner::ApplyRewards;
  using ReportConsensusFault = v3::miner::ReportConsensusFault;
  using WithdrawBalance = v3::miner::WithdrawBalance;
  using ConfirmSectorProofsValid = v3::miner::ConfirmSectorProofsValid;
  using ChangeMultiaddresses = v3::miner::ChangeMultiaddresses;
  using CompactPartitions = v3::miner::CompactPartitions;
  using CompactSectorNumbers = v3::miner::CompactSectorNumbers;
  using ConfirmUpdateWorkerKey = v3::miner::ConfirmUpdateWorkerKey;
  using RepayDebt = v3::miner::RepayDebt;
  using ChangeOwnerAddress = v3::miner::ChangeOwnerAddress;
  using DisputeWindowedPoSt = v3::miner::DisputeWindowedPoSt;

  // extern const ActorExports exports;

}  // namespace fc::vm::actor::builtin::v4::miner
