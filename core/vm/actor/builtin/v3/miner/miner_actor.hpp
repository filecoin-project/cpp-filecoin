/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/sector/sector.hpp"
#include "vm/actor/builtin/v2/miner/miner_actor.hpp"

namespace fc::vm::actor::builtin::v3::miner {
  using libp2p::multi::Multiaddress;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::sector::RegisteredPoStProof;

  struct Construct : ActorMethodBase<1> {
    struct Params {
      Address owner;
      Address worker;
      std::vector<Address> control_addresses;
      RegisteredPoStProof post_proof_type;
      Bytes peer_id;
      std::vector<Multiaddress> multiaddresses;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(Construct::Params,
             owner,
             worker,
             control_addresses,
             post_proof_type,
             peer_id,
             multiaddresses)

  using ControlAddresses = v2::miner::ControlAddresses;
  using ChangeWorkerAddress = v2::miner::ChangeWorkerAddress;
  using ChangePeerId = v2::miner::ChangePeerId;

  struct SubmitWindowedPoSt : ActorMethodBase<5> {
    using Params = v2::miner::SubmitWindowedPoSt::Params;

    ACTOR_METHOD_DECL();
  };

  // TODO implement
  using PreCommitSector = v2::miner::PreCommitSector;
  using ProveCommitSector = v2::miner::ProveCommitSector;
  using ExtendSectorExpiration = v2::miner::ExtendSectorExpiration;
  using TerminateSectors = v2::miner::TerminateSectors;
  using DeclareFaults = v2::miner::DeclareFaults;
  using DeclareFaultsRecovered = v2::miner::DeclareFaultsRecovered;
  using OnDeferredCronEvent = v2::miner::OnDeferredCronEvent;
  using CheckSectorProven = v2::miner::CheckSectorProven;
  using ApplyRewards = v2::miner::ApplyRewards;
  using ReportConsensusFault = v2::miner::ReportConsensusFault;
  using WithdrawBalance = v2::miner::WithdrawBalance;
  using ConfirmSectorProofsValid = v2::miner::ConfirmSectorProofsValid;
  using ChangeMultiaddresses = v2::miner::ChangeMultiaddresses;
  using CompactPartitions = v2::miner::CompactPartitions;
  using CompactSectorNumbers = v2::miner::CompactSectorNumbers;
  using ConfirmUpdateWorkerKey = v2::miner::ConfirmUpdateWorkerKey;
  using RepayDebt = v2::miner::RepayDebt;
  using ChangeOwnerAddress = v2::miner::ChangeOwnerAddress;

  struct DisputeWindowedPoSt : ActorMethodBase<24> {
    using Params = Address;
    ACTOR_METHOD_DECL();
  };

  extern const ActorExports exports;

}  // namespace fc::vm::actor::builtin::v3::miner
