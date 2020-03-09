/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MARKET_ACTOR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MARKET_ACTOR_HPP

#include "filecoin/codec/cbor/streams_annotation.hpp"
#include "filecoin/primitives/types.hpp"
#include "filecoin/vm/actor/actor_method.hpp"

namespace fc::vm::actor::builtin::market {
  using primitives::ChainEpoch;
  using primitives::DealId;
  using primitives::DealWeight;
  using primitives::SectorSize;

  struct VerifyDealsOnSectorProveCommit : ActorMethodBase<6> {
    struct Params {
      std::vector<DealId> deals;
      ChainEpoch sector_expiry;
    };
    using Result = DealWeight;
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(VerifyDealsOnSectorProveCommit::Params, deals, sector_expiry)

  struct OnMinerSectorsTerminate : ActorMethodBase<7> {
    struct Params {
      std::vector<DealId> deals;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(OnMinerSectorsTerminate::Params, deals)

  struct ComputeDataCommitment : ActorMethodBase<8> {
    struct Params {
      std::vector<DealId> deals;
      SectorSize sector_size;
    };
    using Result = CID;
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(ComputeDataCommitment::Params, deals, sector_size)
}  // namespace fc::vm::actor::builtin::market

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MARKET_ACTOR_HPP
