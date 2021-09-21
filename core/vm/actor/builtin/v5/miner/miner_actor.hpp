/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v4/miner/miner_actor.hpp"

#include "vm/actor/builtin/types/miner/sector_info.hpp"

namespace fc::vm::actor::builtin::v5::miner {
  using primitives::RleBitset;
  using types::miner::SectorPreCommitInfo;

  // TODO implement
  using Construct = v4::miner::Construct;
  using ControlAddresses = v4::miner::ControlAddresses;
  using ChangeWorkerAddress = v4::miner::ChangeWorkerAddress;
  using ChangePeerId = v4::miner::ChangePeerId;
  using SubmitWindowedPoSt = v4::miner::SubmitWindowedPoSt;
  using PreCommitSector = v4::miner::PreCommitSector;
  using ProveCommitSector = v4::miner::ProveCommitSector;
  using ExtendSectorExpiration = v4::miner::ExtendSectorExpiration;
  using TerminateSectors = v4::miner::TerminateSectors;
  using DeclareFaults = v4::miner::DeclareFaults;
  using DeclareFaultsRecovered = v4::miner::DeclareFaultsRecovered;
  using OnDeferredCronEvent = v4::miner::OnDeferredCronEvent;
  using CheckSectorProven = v4::miner::CheckSectorProven;
  using ApplyRewards = v4::miner::ApplyRewards;
  using ReportConsensusFault = v4::miner::ReportConsensusFault;
  using WithdrawBalance = v4::miner::WithdrawBalance;
  using ConfirmSectorProofsValid = v4::miner::ConfirmSectorProofsValid;
  using ChangeMultiaddresses = v4::miner::ChangeMultiaddresses;
  using CompactPartitions = v4::miner::CompactPartitions;
  using CompactSectorNumbers = v4::miner::CompactSectorNumbers;
  using ConfirmUpdateWorkerKey = v4::miner::ConfirmUpdateWorkerKey;
  using RepayDebt = v4::miner::RepayDebt;
  using ChangeOwnerAddress = v4::miner::ChangeOwnerAddress;
  using DisputeWindowedPoSt = v4::miner::DisputeWindowedPoSt;

  /**
   * Collects and stores precommit messages to make a packaged sending of a
   * several messages within one transaction which reduces the general amount of
   * transactions in the network with reduction of a gas fee for transactions.
   */
  struct PreCommitBatch : ActorMethodBase<25> {
    struct Params {
      std::vector<SectorPreCommitInfo> sectors;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(PreCommitBatch::Params, sectors);

  struct ProveCommitAggregate : ActorMethodBase<26> {
    struct Params {
      RleBitset sectors;
      Bytes proof;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(ProveCommitAggregate::Params, sectors, proof);

  // extern const ActorExports exports;

}  // namespace fc::vm::actor::builtin::v5::miner
