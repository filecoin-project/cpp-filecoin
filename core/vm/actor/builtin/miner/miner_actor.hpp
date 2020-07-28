/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MINER_MINER_ACTOR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MINER_MINER_ACTOR_HPP

#include "codec/cbor/streams_annotation.hpp"
#include "primitives/address/address.hpp"
#include "vm/actor/actor_method.hpp"
#include "vm/actor/builtin/miner/types.hpp"

namespace fc::vm::actor::builtin::miner {
  using primitives::sector::PoStProof;

  struct Construct : ActorMethodBase<1> {
    struct Params {
      Address owner;
      Address worker;
      RegisteredProof seal_proof_type;
      PeerId peer_id{codec::cbor::kDefaultT<PeerId>()};
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(Construct::Params, owner, worker, seal_proof_type, peer_id)

  struct ControlAddresses : ActorMethodBase<2> {
    struct Result {
      Address owner;
      Address worker;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(ControlAddresses::Result, owner, worker)

  struct ChangeWorkerAddress : ActorMethodBase<3> {
    struct Params {
      Address new_worker;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(ChangeWorkerAddress::Params, new_worker)

  struct ChangePeerId : ActorMethodBase<4> {
    struct Params {
      PeerId new_id{codec::cbor::kDefaultT<PeerId>()};
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(ChangePeerId::Params, new_id)

  struct SubmitWindowedPoSt : ActorMethodBase<5> {
    struct Params {
      uint64_t deadline{};
      std::vector<uint64_t> partitions;
      std::vector<PoStProof> proofs;
      RleBitset skipped;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(SubmitWindowedPoSt::Params, deadline, partitions, proofs, skipped)

  struct PreCommitSector : ActorMethodBase<6> {
    using Params = SectorPreCommitInfo;
    ACTOR_METHOD_DECL();
  };

  struct ProveCommitSector : ActorMethodBase<7> {
    struct Params {
      SectorNumber sector;
      Proof proof;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(ProveCommitSector::Params, sector, proof)

  struct ExtendSectorExpiration : ActorMethodBase<8> {
    struct Params {
      SectorNumber sector;
      ChainEpoch new_expiration;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(ExtendSectorExpiration::Params, sector, new_expiration)

  struct TerminateSectors : ActorMethodBase<9> {
    struct Params {
      RleBitset sectors;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(TerminateSectors::Params, sectors)

  struct DeclareFaults : ActorMethodBase<10> {
    struct FaultDeclaration {
      uint64_t deadline;
      RleBitset sectors;
    };
    struct Params {
      std::vector<FaultDeclaration> faults;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(DeclareFaults::FaultDeclaration, deadline, sectors)
  CBOR_TUPLE(DeclareFaults::Params, faults)

  struct DeclareFaultsRecovered : ActorMethodBase<11> {
    struct RecoveryDeclaration {
      uint64_t deadline;
      RleBitset sectors;
    };
    struct Params {
      std::vector<RecoveryDeclaration> recoveries;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(DeclareFaultsRecovered::RecoveryDeclaration, deadline, sectors)
  CBOR_TUPLE(DeclareFaultsRecovered::Params, recoveries)

  struct OnDeferredCronEvent : ActorMethodBase<12> {
    using Params = CronEventPayload;
    ACTOR_METHOD_DECL();
  };

  struct CheckSectorProven : ActorMethodBase<13> {
    struct Params {
      SectorNumber sector;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(CheckSectorProven::Params, sector)

  struct AddLockedFund : ActorMethodBase<14> {
    using Params = TokenAmount;
    ACTOR_METHOD_DECL();
  };

  struct ReportConsensusFault : ActorMethodBase<15> {
    struct Params {
      Buffer block1, block2, extra;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(ReportConsensusFault::Params, block1, block2, extra)

  struct WithdrawBalance : ActorMethodBase<16> {
    struct Params {
      TokenAmount amount;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(WithdrawBalance::Params, amount)

  extern const ActorExports exports;

}  // namespace fc::vm::actor::builtin::miner

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MINER_MINER_ACTOR_HPP
