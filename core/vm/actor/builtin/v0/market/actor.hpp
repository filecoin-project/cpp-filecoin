/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor_method.hpp"
#include "vm/actor/builtin/v0/market/market_actor_state.hpp"

namespace fc::vm::actor::builtin::v0::market {
  using primitives::DealWeight;
  using primitives::sector::RegisteredSealProof;
  using runtime::Runtime;

  struct Construct : ActorMethodBase<1> {
    ACTOR_METHOD_DECL();
  };

  /**
   * Deposits the received value into the balance held in escrow
   */
  struct AddBalance : ActorMethodBase<2> {
    using Params = Address;
    ACTOR_METHOD_DECL();
  };

  /**
   * Attempt to withdraw the specified amount from the balance held in escrow.
   * If less than the specified amount is available, yields the entire available
   * balance.
   */
  struct WithdrawBalance : ActorMethodBase<3> {
    struct Params {
      Address address;
      TokenAmount amount;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(WithdrawBalance::Params, address, amount)

  /**
   * Publish a new set of storage deals (not yet included in a sector).
   */
  struct PublishStorageDeals : ActorMethodBase<4> {
    struct Params {
      std::vector<ClientDealProposal> deals;
    };
    struct Result {
      std::vector<DealId> deals;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(PublishStorageDeals::Params, deals)
  CBOR_TUPLE(PublishStorageDeals::Result, deals)

  /**
   * Verify that a given set of storage deals is valid for a sector currently
   * being PreCommitted and return DealWeight of the set of storage deals given.
   * The weight is defined as the sum, over all deals in the set, of the product
   * of deal size and duration.
   */
  struct VerifyDealsForActivation : ActorMethodBase<5> {
    struct Params {
      std::vector<DealId> deals;
      ChainEpoch sector_expiry;
      ChainEpoch sector_start;
    };
    struct Result {
      DealWeight deal_weight;
      DealWeight verified_deal_weight;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(VerifyDealsForActivation::Params,
             deals,
             sector_expiry,
             sector_start)
  CBOR_TUPLE(VerifyDealsForActivation::Result,
             deal_weight,
             verified_deal_weight)

  /**
   * Verify that a given set of storage deals is valid for a sector currently
   * being ProveCommitted, update the market's internal state accordingly.
   */
  struct ActivateDeals : ActorMethodBase<6> {
    struct Params {
      std::vector<DealId> deals;
      ChainEpoch sector_expiry;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(ActivateDeals::Params, deals, sector_expiry)

  /**
   * Terminate a set of deals in response to their containing sector being
   * terminated. Slash provider collateral, refund client collateral, and refund
   * partial unpaid escrow amount to client.
   */
  struct OnMinerSectorsTerminate : ActorMethodBase<7> {
    struct Params {
      ChainEpoch epoch;
      std::vector<DealId> deals;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(OnMinerSectorsTerminate::Params, epoch, deals)

  struct ComputeDataCommitment : ActorMethodBase<8> {
    struct Params {
      std::vector<DealId> deals;
      RegisteredSealProof sector_type;
    };
    using Result = CID;
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(ComputeDataCommitment::Params, deals, sector_type)

  struct CronTick : ActorMethodBase<9> {
    ACTOR_METHOD_DECL();
  };

  extern const ActorExports exports;

}  // namespace fc::vm::actor::builtin::v0::market
