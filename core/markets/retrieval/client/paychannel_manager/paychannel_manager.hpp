/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_CLIENT_PAYCHANNEL_MANAGER_PAYCHANNEL_MANAGER_HPP
#define CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_CLIENT_PAYCHANNEL_MANAGER_PAYCHANNEL_MANAGER_HPP

#include "primitives/address/address.hpp"
#include "primitives/types.hpp"
#include "vm/actor/builtin/payment_channel/payment_channel_actor_state.hpp"

namespace fc::markets::retrieval::client::paychannel_manager {
  using primitives::TokenAmount;
  using primitives::address::Address;
  using vm::actor::builtin::payment_channel::LaneId;

  class PayChannelManager {
   public:
    /**
     * Sets up a new payment channel if one does not exist between a client and
     * a miner and ensures the client has the given amount of funds available in
     * the channel
     * @param client address
     * @param miner address
     * @return payment channel address
     */
    outcome::result<Address> getOrCreatePaymentChannel(
        const Address &client,
        const Address &miner,
        const TokenAmount &amount_available);

    /**
     * Allocate late creates a lane within a payment channel so that calls to
     * CreatePaymentVoucher will automatically make vouchers only for the
     * difference in total
     * @param payment channel address
     * @return payment lane id
     */
    outcome::result<LaneId> allocateLane(const Address &channel);

    /**
     * Creates a new payment voucher in the given lane for a given payment
     * channel so that all the payment vouchers in the lane add up to the given
     * amount (so the payment voucher will be for the difference)
     */
    outcome::result<CID> createPaymentVoucher(const Address &channel,
                                              const LaneID &lane,
                                              const TokenAmount &amount);
  };
}  // namespace fc::markets::retrieval::client::paychannel_manager

#endif  // CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_CLIENT_PAYCHANNEL_MANAGER_PAYCHANNEL_MANAGER_HPP
