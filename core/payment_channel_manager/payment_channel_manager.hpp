/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/full_node/node_api.hpp"
#include "primitives/address/address.hpp"
#include "primitives/types.hpp"
#include "vm/actor/builtin/types/payment_channel/voucher.hpp"

namespace fc::payment_channel_manager {
  using api::AddChannelInfo;
  using api::FullNodeApi;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using vm::actor::builtin::types::payment_channel::LaneId;
  using vm::actor::builtin::types::payment_channel::SignedVoucher;

  /**
   * PaymentChannelManager is a module responsible for off-chain payments via
   * payment channels. Tracks on-chain channel actor states, and off-chain lane
   * states and vouchers. Providers API for node. Used for retrieval markets.
   */
  class PaymentChannelManager {
   public:
    virtual ~PaymentChannelManager() = default;

    /**
     * Allocate late creates a lane within a payment channel so that calls to
     * CreatePaymentVoucher will automatically make vouchers only for the
     * difference in total
     * @param payment channel address
     * @return payment lane id
     */
    virtual outcome::result<LaneId> allocateLane(
        const Address &channel_address) = 0;

    /**
     * Creates a new payment voucher in the given lane for a given payment
     * channel so that all the payment vouchers in the lane add up to the given
     * amount (so the payment voucher will be for the difference)
     * @param channel address
     * @param lane id
     */
    virtual outcome::result<SignedVoucher> createPaymentVoucher(
        const Address &channel_address,
        const LaneId &lane,
        const TokenAmount &amount) = 0;

    /**
     * Adds existing payment voucher
     * @param channel_address
     * @param voucher
     * @return amount to redeem
     */
    virtual outcome::result<TokenAmount> savePaymentVoucher(
        const Address &channel_address, const SignedVoucher &voucher) = 0;

    /**
     * Validates signed voucher
     * @param channel_address payment channel actor address
     * @param voucher to validate
     * @return Error if voucher is not valid or nothing othervise
     */
    virtual outcome::result<void> validateVoucher(
        const Address &channel_address, const SignedVoucher &voucher) const = 0;

    /**
     * Adds payment channel manager methods to API
     * @param api to add methods
     */
    virtual void makeApi(FullNodeApi &api) = 0;
  };
}  // namespace fc::payment_channel_manager
