/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor_method.hpp"

#include "primitives/rle_bitset/rle_bitset.hpp"
#include "vm/actor/builtin/types/market/deal.hpp"
#include "vm/actor/builtin/types/market/sector_data_spec.hpp"
#include "vm/actor/builtin/types/market/sector_deals.hpp"
#include "vm/actor/builtin/types/market/sector_weights.hpp"

namespace fc::vm::actor::builtin {
  using primitives::ChainEpoch;
  using primitives::DealId;
  using primitives::DealWeight;
  using primitives::RleBitset;
  using primitives::sector::RegisteredSealProof;
  using types::market::ClientDealProposal;
  using types::market::SectorDataSpec;
  using types::market::SectorDeals;
  using types::market::SectorWeights;

  // These methods must be actual with the last version of actors

  struct MarketActor {
    enum class Method : MethodNumber {
      kConstruct = 1,
      kAddBalance,
      kWithdrawBalance,
      kPublishStorageDeals,
      kVerifyDealsForActivation,
      kActivateDeals,
      kOnMinerSectorsTerminate,
      kComputeDataCommitment,
      kCronTick,
    }

    struct Construct : ActorMethodBase<Method::kConstruct> {};

    struct AddBalance : ActorMethodBase<Method::kAddBalance> {
      using Params = Address;
    };

    struct WithdrawBalance : ActorMethodBase<Method::kWithdrawBalance> {
      struct Params {
        Address address;
        TokenAmount amount;

        inline bool operator==(const Params &other) const {
          return address == other.address && amount == other.amount;
        }

        inline bool operator!=(const Params &other) const {
          return !(*this == other);
        }
      };
    };
    CBOR_TUPLE(WithdrawBalance::Params, address, amount)

    struct PublishStorageDeals : ActorMethodBase<Method::kPublishStorageDeals> {
      struct Params {
        std::vector<ClientDealProposal> deals;

        inline bool operator==(const Params &other) const {
          return deals == other.deals;
        }

        inline bool operator!=(const Params &other) const {
          return !(*this == other);
        }
      };

      struct Result {
        std::vector<DealId> deals;
        RleBitset valid_deals;

        inline bool operator==(const Result &other) const {
          return deals == other.deals && valid_deals == other.valid_deals;
        }

        inline bool operator!=(const Result &other) const {
          return !(*this == other);
        }
      };
    };
    CBOR_TUPLE(PublishStorageDeals::Params, deals)
    CBOR_TUPLE(PublishStorageDeals::Result, deals, valid_deals)

    struct VerifyDealsForActivation
        : ActorMethodBase<Method::kVerifyDealsForActivation> {
      struct Params {
        std::vector<SectorDeals> sectors;

        inline bool operator==(const Params &other) const {
          return sectors == other.sectors;
        }

        inline bool operator!=(const Params &other) const {
          return !(*this == other);
        }
      };
      struct Result {
        std::vector<SectorWeights> sectors;

        inline bool operator==(const Result &other) const {
          return !(*this == other);
        }
      };
    };
    CBOR_TUPLE(VerifyDealsForActivation::Params, sectors)
    CBOR_TUPLE(VerifyDealsForActivation::Result, sectors)

    struct ActivateDeals : ActorMethodBase<Method::kActivateDeals> {
      struct Params {
        std::vector<DealId> deals;
        ChainEpoch sector_expiry{};

        inline bool operator==(const Params &other) const {
          return deals == other.deals && sector_expiry == other.sector_expiry;
        }

        inline bool operator!=(const Params &other) const {
          return !(*this == other);
        }
      };
    };
    CBOR_TUPLE(ActivateDeals::Params, deals, sector_expiry)

    struct OnMinerSectorsTerminate
        : ActorMethodBase<Method::kOnMinerSectorsTerminate> {
      struct Params {
        ChainEpoch epoch{};
        std::vector<DealId> deals;

        inline bool operator==(const Params &other) const {
          return epoch == other.epoch && deals == other.deals;
        }

        inline bool operator!=(const Params &other) const {
          return !(*this == other);
        }
      };
    };
    CBOR_TUPLE(OnMinerSectorsTerminate::Params, epoch, deals)

    struct ComputeDataCommitment
        : ActorMethodBase<Method::kComputeDataCommitment> {
      struct Params {
        std::vector<SectorDataSpec> inputs;

        inline bool operator==(const Params &other) const {
          return inputs == other.inputs;
        }

        inline bool operator!=(const Params &other) const {
          return !(*this == other);
        }
      };

      struct Result {
        std::vector<CID> commds;

        inline bool operator==(const Result &other) const {
          return commds == other.commds;
        }

        inline bool operator!=(const Result &other) const {
          return !(*this == other);
        }
      };
    };
    CBOR_TUPLE(ComputeDataCommitment::Params, inputs)
    CBOR_TUPLE(ComputeDataCommitment::Result, commds)

    struct CronTick : ActorMethodBase<Method::kCronTick> {};
  };

}  // namespace fc::vm::actor::builtin
