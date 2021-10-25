/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v0/miner/miner_actor.hpp"

namespace fc::vm::actor::builtin::v2::miner {
  using primitives::TokenAmount;

  struct Construct : ActorMethodBase<1> {
    using Params = v0::miner::Construct::Params;

    ACTOR_METHOD_DECL();
  };

  using ControlAddresses = v0::miner::ControlAddresses;

  struct ChangeWorkerAddress : ActorMethodBase<3> {
    using Params = v0::miner::ChangeWorkerAddress::Params;

    ACTOR_METHOD_DECL();
  };

  using ChangePeerId = v0::miner::ChangePeerId;

  struct SubmitWindowedPoSt : ActorMethodBase<5> {
    using Params = v0::miner::SubmitWindowedPoSt::Params;

    ACTOR_METHOD_DECL();
  };

  struct PreCommitSector : ActorMethodBase<6> {
    using Params = v0::miner::PreCommitSector::Params;

    ACTOR_METHOD_DECL();
  };

  struct ProveCommitSector : ActorMethodBase<7> {
    using Params = v0::miner::ProveCommitSector::Params;

    ACTOR_METHOD_DECL();
  };

  // TODO implement
  using ExtendSectorExpiration = v0::miner::ExtendSectorExpiration;
  using TerminateSectors = v0::miner::TerminateSectors;
  using DeclareFaults = v0::miner::DeclareFaults;
  using DeclareFaultsRecovered = v0::miner::DeclareFaultsRecovered;
  using OnDeferredCronEvent = v0::miner::OnDeferredCronEvent;
  using CheckSectorProven = v0::miner::CheckSectorProven;

  struct ApplyRewards : ActorMethodBase<14> {
    struct Params {
      TokenAmount reward;
      TokenAmount penalty;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(ApplyRewards::Params, reward, penalty)

  using ReportConsensusFault = v0::miner::ReportConsensusFault;
  using WithdrawBalance = v0::miner::WithdrawBalance;
  using ConfirmSectorProofsValid = v0::miner::ConfirmSectorProofsValid;
  using ChangeMultiaddresses = v0::miner::ChangeMultiaddresses;
  using CompactPartitions = v0::miner::CompactPartitions;
  using CompactSectorNumbers = v0::miner::CompactSectorNumbers;

  /**
   * Triggers a worker address change if a change has been requested and its
   * effective epoch has arrived
   */
  struct ConfirmUpdateWorkerKey : ActorMethodBase<21> {
    ACTOR_METHOD_DECL();
  };

  struct RepayDebt : ActorMethodBase<22> {
    ACTOR_METHOD_DECL();
  };

  /**
   * Proposes or confirms a change of owner address
   *
   * If invoked by the current owner, proposes a new owner address for
   * confirmation. If the proposed address is the current owner address,
   * revokes any existing proposal. If invoked by the previously proposed
   * address, with the same proposal, changes the current owner address to be
   * that proposed address.
   */
  struct ChangeOwnerAddress : ActorMethodBase<23> {
    using Params = Address;
    ACTOR_METHOD_DECL();
  };

  extern const ActorExports exports;

}  // namespace fc::vm::actor::builtin::v2::miner
