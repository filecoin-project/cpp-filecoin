/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MINER_MINER_ACTOR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MINER_MINER_ACTOR_HPP

#include "filecoin/codec/cbor/streams_annotation.hpp"
#include "filecoin/primitives/address/address.hpp"
#include "filecoin/primitives/address/address_codec.hpp"
#include "filecoin/vm/actor/actor_method.hpp"
#include "filecoin/vm/actor/builtin/miner/types.hpp"

namespace fc::vm::actor::builtin::miner {

  constexpr MethodNumber kSubmitElectionPoStMethodNumber{20};

  struct Construct : ActorMethodBase<1> {
    struct Params {
      Address owner;
      Address worker;
      SectorSize sector_size;
      PeerId peer_id;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(Construct::Params, owner, worker, sector_size, peer_id)

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
      PeerId new_id;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(ChangePeerId::Params, new_id)

  struct SubmitWindowedPoSt : ActorMethodBase<5> {
    using Params = OnChainPoStVerifyInfo;
    ACTOR_METHOD_DECL();
  };

  struct OnDeleteMiner : ActorMethodBase<6> {
    ACTOR_METHOD_DECL();
  };

  struct PreCommitSector : ActorMethodBase<7> {
    using Params = SectorPreCommitInfo;
    ACTOR_METHOD_DECL();
  };

  struct ProveCommitSector : ActorMethodBase<8> {
    struct Params {
      SectorNumber sector;
      SealProof proof;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(ProveCommitSector::Params, sector, proof)

  struct ExtendSectorExpiration : ActorMethodBase<9> {
    struct Params {
      SectorNumber sector;
      ChainEpoch new_expiration;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(ExtendSectorExpiration::Params, sector, new_expiration)

  struct TerminateSectors : ActorMethodBase<10> {
    struct Params {
      boost::optional<RleBitset> sectors;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(TerminateSectors::Params, sectors)

  struct DeclareTemporaryFaults : ActorMethodBase<11> {
    struct Params {
      RleBitset sectors;
      EpochDuration duration;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(DeclareTemporaryFaults::Params, sectors, duration)

  struct OnDeferredCronEvent : ActorMethodBase<12> {
    struct Params {
      Buffer callback_payload;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(OnDeferredCronEvent::Params, callback_payload)

  struct CheckSectorProven : ActorMethodBase<13> {
    struct Params {
      SectorNumber sector;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(CheckSectorProven::Params, sector)

  extern const ActorExports exports;

}  // namespace fc::vm::actor::builtin::miner

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MINER_MINER_ACTOR_HPP
