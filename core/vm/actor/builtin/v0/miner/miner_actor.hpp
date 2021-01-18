/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MINER_MINER_ACTOR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MINER_MINER_ACTOR_HPP

#include "codec/cbor/streams_annotation.hpp"
#include "primitives/address/address.hpp"
#include "vm/actor/actor_method.hpp"
#include "vm/actor/builtin/v0/miner/types.hpp"

// TODO: update methods

namespace fc::vm::actor::builtin::v0::miner {
  using primitives::sector::PoStProof;

  struct Construct : ActorMethodBase<1> {
    struct Params {
      Address owner;
      Address worker;
      RegisteredSealProof seal_proof_type;
      Buffer peer_id;
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
      Buffer new_id;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(ChangePeerId::Params, new_id)

  struct SubmitWindowedPoSt : ActorMethodBase<5> {
    struct Params {
      struct Partition {
        uint64_t index;
        RleBitset skipped;
      };

      uint64_t deadline{};
      std::vector<Partition> partitions;
      std::vector<PoStProof> proofs;
      ChainEpoch chain_commit_epoch;
      Randomness chain_commit_rand;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(SubmitWindowedPoSt::Params::Partition, index, skipped)
  CBOR_TUPLE(SubmitWindowedPoSt::Params,
             deadline,
             partitions,
             proofs,
             chain_commit_epoch,
             chain_commit_rand)

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
    struct Declaration {
      uint64_t deadline;
      uint64_t partition;
      RleBitset sectors;
    };
    struct Params {
      std::vector<Declaration> declarations;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(DeclareFaults::Declaration, deadline, partition, sectors)
  CBOR_TUPLE(DeclareFaults::Params, declarations)

  struct DeclareFaultsRecovered : ActorMethodBase<11> {
    using Params = DeclareFaults::Params;
    ACTOR_METHOD_DECL();
  };

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

}  // namespace fc::vm::actor::builtin::v0::miner

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MINER_MINER_ACTOR_HPP
