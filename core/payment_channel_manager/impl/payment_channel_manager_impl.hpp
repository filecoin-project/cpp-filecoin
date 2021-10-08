/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <shared_mutex>

#include "api/full_node/node_api.hpp"
#include "common/buffer.hpp"
#include "payment_channel_manager/payment_channel_manager.hpp"
#include "storage/ipfs/datastore.hpp"
#include "vm/actor/builtin/states/payment_channel/payment_channel_actor_state.hpp"

namespace fc::payment_channel_manager {
  using api::FullNodeApi;
  using common::Buffer;
  using primitives::Nonce;
  using vm::actor::builtin::states::PaymentChannelActorStatePtr;
  using vm::actor::builtin::types::payment_channel::SignedVoucher;

  struct ChannelInfo {
    Address channel_actor;
    Address control;
    Address target;
    // TODO(turuslan): FIL-420 check cache memory usage
    std::map<LaneId, std::vector<SignedVoucher>> lanes;
    LaneId next_lane;
  };

  class PaymentChannelManagerImpl
      : public PaymentChannelManager,
        public std::enable_shared_from_this<PaymentChannelManagerImpl> {
   public:
    PaymentChannelManagerImpl(std::shared_ptr<FullNodeApi> api,
                              std::shared_ptr<Ipld> ipld);

    void getOrCreatePaymentChannel(const Address &client,
                                   const Address &miner,
                                   const TokenAmount &amount_available,
                                   AddChannelInfoCb cb) override;

    outcome::result<LaneId> allocateLane(const Address &channel) override;

    outcome::result<SignedVoucher> createPaymentVoucher(
        const Address &channel_address,
        const LaneId &lane,
        const TokenAmount &amount) override;

    outcome::result<TokenAmount> savePaymentVoucher(
        const Address &channel_address, const SignedVoucher &voucher) override;

    outcome::result<void> validateVoucher(
        const Address &channel_address,
        const SignedVoucher &voucher) const override;

    boost::optional<Address> findChannel(const Address &control,
                                         const Address &target) const override;

    void makeApi(FullNodeApi &api) override;

   private:
    /**
     * Save channel state to local offchain storage
     * @param channel_actor_address channel address
     * @param control from address
     * @param target to address
     */
    void saveChannel(const Address &channel_actor_address,
                     const Address &control,
                     const Address &target);

    /**
     * Sends funds
     * @param to address
     * @param from address
     * @param amount to send
     * @return CID add funds message
     */
    outcome::result<CID> addFunds(const Address &to,
                                  const Address &from,
                                  const TokenAmount &amount);

    outcome::result<CID> createPaymentChannelActor(const Address &to,
                                                   const Address &from,
                                                   const TokenAmount &amount);

    /**
     * Loads payment channel actor state
     * @param channel_address payment channel actor address
     * @return payment channel actor state
     */
    outcome::result<PaymentChannelActorStatePtr> loadPaymentChannelActorState(
        const Address &channel_address) const;

    /**
     * Get next nonce for lane in channel info
     * @param channel info
     * @param lane id
     * @return next nonce
     */
    outcome::result<Nonce> getNextNonce(const ChannelInfo &channel,
                                        const LaneId &lane) const;

    std::shared_ptr<FullNodeApi> api_;
    std::shared_ptr<Ipld> ipld_;
    /**
     * Channel state offchain storage is a map (channel_actor_address ->
     * ChannelInfo)
     */
    // TODO(turuslan): FIL-420 check cache memory usage
    std::map<Address, ChannelInfo> channels_;
    mutable std::shared_mutex channels_mutex_;
  };

}  // namespace fc::payment_channel_manager
