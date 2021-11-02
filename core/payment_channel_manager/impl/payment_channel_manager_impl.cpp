/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "payment_channel_manager/impl/payment_channel_manager_impl.hpp"

#include "cbor_blake/ipld_version.hpp"
#include "payment_channel_manager/impl/payment_channel_manager_error.hpp"
#include "vm/actor/builtin/v0/init/init_actor.hpp"
#include "vm/actor/builtin/v0/payment_channel/payment_channel_actor.hpp"
#include "vm/actor/codes.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

namespace fc::payment_channel_manager {
  using api::AddChannelInfo;
  using crypto::signature::Signature;
  using primitives::Nonce;
  using vm::VMExitCode;
  using vm::actor::kInitAddress;
  using vm::actor::MethodParams;
  using vm::message::kDefaultGasLimit;
  using vm::message::kDefaultGasPrice;
  using vm::message::UnsignedMessage;
  using vm::state::StateTreeImpl;
  using InitActorExec = vm::actor::builtin::v0::init::Exec;
  using PaymentChannelConstruct =
      vm::actor::builtin::v0::payment_channel::Construct;

  PaymentChannelManagerImpl::PaymentChannelManagerImpl(
      std::shared_ptr<FullNodeApi> api, std::shared_ptr<Ipld> ipld)
      : api_{std::move(api)}, ipld_{std::move(ipld)} {}

  outcome::result<LaneId> PaymentChannelManagerImpl::allocateLane(
      const Address &channel_address) {
    std::unique_lock lock(channels_mutex_);
    auto lookup = channels_.find(channel_address);
    if (lookup == channels_.end()) {
      return PaymentChannelManagerError::kChannelNotFound;
    }
    lookup->second.lanes.try_emplace(lookup->second.next_lane++);
    return lookup->second.next_lane;
  }

  outcome::result<SignedVoucher>
  PaymentChannelManagerImpl::createPaymentVoucher(
      const Address &channel_address,
      const LaneId &lane,
      const TokenAmount &amount) {
    SignedVoucher new_voucher;
    new_voucher.lane = lane;
    new_voucher.amount = amount;

    std::unique_lock lock(channels_mutex_);
    auto lookup_channel = channels_.find(channel_address);
    if (lookup_channel == channels_.end()) {
      return PaymentChannelManagerError::kChannelNotFound;
    }
    OUTCOME_TRYA(new_voucher.nonce, getNextNonce(lookup_channel->second, lane));

    // sign voucher
    OUTCOME_TRY(voucher_bytes, codec::cbor::encode(new_voucher));
    OUTCOME_TRY(
        signature,
        api_->WalletSign(lookup_channel->second.control, voucher_bytes));
    new_voucher.signature_bytes = signature.toBytes();

    OUTCOME_TRY(validateVoucher(channel_address, new_voucher));
    lookup_channel->second.lanes[lane].push_back(new_voucher);
    return new_voucher;
  }

  outcome::result<TokenAmount> PaymentChannelManagerImpl::savePaymentVoucher(
      const Address &channel_address, const SignedVoucher &voucher) {
    OUTCOME_TRY(validateVoucher(channel_address, voucher));
    OUTCOME_TRY(payment_channel_actor_state,
                loadPaymentChannelActorState(channel_address));
    std::unique_lock lock(channels_mutex_);

    // add channel to local storage if hasn't been added yet
    auto channel_lookup = channels_.find(channel_address);
    if (channel_lookup == channels_.end()) {
      saveChannel(channel_address,
                  payment_channel_actor_state->from,
                  payment_channel_actor_state->to);
      channel_lookup = channels_.find(channel_address);
    }

    // insert if no duplicates
    auto lanes = channel_lookup->second.lanes[voucher.lane];
    if (find(lanes.begin(), lanes.end(), voucher) == lanes.end()) {
      channel_lookup->second.lanes[voucher.lane].push_back(voucher);
    }

    // get redeemed
    TokenAmount redeemed{0};
    OUTCOME_TRY(lane_lookup,
                payment_channel_actor_state->lanes.tryGet(voucher.lane));
    if (lane_lookup) {
      redeemed = lane_lookup->redeem;
    }

    return voucher.amount - redeemed;
  }

  outcome::result<void> PaymentChannelManagerImpl::validateVoucher(
      const Address &channel_address, const SignedVoucher &voucher) const {
    OUTCOME_TRY(payment_channel_actor_state,
                loadPaymentChannelActorState(channel_address));

    // check signature
    auto signature = Signature::fromBytes(voucher.signature_bytes.get());
    if (!signature) {
      return PaymentChannelManagerError::kWrongSignature;
    }
    auto voucher_to_verify = voucher;
    voucher_to_verify.signature_bytes = boost::none;
    OUTCOME_TRY(bytes_to_verify, codec::cbor::encode(voucher_to_verify));
    OUTCOME_TRY(verified,
                api_->WalletVerify(payment_channel_actor_state->from,
                                   bytes_to_verify,
                                   signature.value()));
    if (!verified) {
      return PaymentChannelManagerError::kWrongSignature;
    }

    // check lane nonce if any lane exists
    auto voucher_send_amount = voucher.amount;
    OUTCOME_TRY(lane_lookup,
                payment_channel_actor_state->lanes.tryGet(voucher.lane));
    if (lane_lookup) {
      if (lane_lookup->nonce >= voucher.nonce) {
        return PaymentChannelManagerError::kWrongNonce;
      }
      if (lane_lookup->redeem >= voucher.amount) {
        return PaymentChannelManagerError::kAlreadyRedeemed;
      }
      voucher_send_amount -= lane_lookup->redeem;
    }

    // check amount
    auto total_amoun =
        payment_channel_actor_state->to_send + voucher_send_amount;
    OUTCOME_TRY(payment_channel_actor_balance,
                api_->WalletBalance(channel_address));
    if (payment_channel_actor_balance < total_amoun) {
      return PaymentChannelManagerError::kInsufficientFunds;
    }

    if (!voucher.merges.empty()) {
      // TODO don't currently support paych lane merges
      return ERROR_TEXT(
          "PaymentChannelManagerImpl::validateVoucher: don't currently support "
          "paych lane merges");
    }

    return outcome::success();
  }

  void PaymentChannelManagerImpl::saveChannel(
      const Address &channel_actor_address,
      const Address &control,
      const Address &target) {
    ChannelInfo channel_info{.channel_actor = channel_actor_address,
                             .control = control,
                             .target = target,
                             .lanes = {},
                             .next_lane = 0};
    channels_[channel_actor_address] = channel_info;
  }

  void PaymentChannelManagerImpl::makeApi(FullNodeApi &api) {
    api.PaychAllocateLane =
        [self{shared_from_this()}](auto &&channel) -> outcome::result<LaneId> {
      return self->allocateLane(channel);
    };
    api.PaychVoucherAdd = [self{shared_from_this()}](
                              auto &&channel,
                              auto &&voucher,
                              auto &&proof,
                              auto &&delta) -> outcome::result<TokenAmount> {
      return self->savePaymentVoucher(channel, voucher);
    };
    api.PaychVoucherCheckValid = [self{shared_from_this()}](
                                     auto &&channel,
                                     auto &&voucher) -> outcome::result<void> {
      return self->validateVoucher(channel, voucher);
    };
    api.PaychVoucherCreate = [self{shared_from_this()}](
                                 auto &&channel, auto &&amount, auto &&lane)
        -> outcome::result<SignedVoucher> {
      return self->createPaymentVoucher(channel, lane, amount);
    };
  }

  outcome::result<PaymentChannelActorStatePtr>
  PaymentChannelManagerImpl::loadPaymentChannelActorState(
      const Address &channel_address) const {
    OUTCOME_TRY(tipset, api_->ChainHead());
    auto ipld{withVersion(ipld_, tipset->epoch())};
    auto state_tree =
        std::make_shared<StateTreeImpl>(ipld, tipset->getParentStateRoot());
    OUTCOME_TRY(actor, state_tree->get(channel_address));
    return getCbor<PaymentChannelActorStatePtr>(ipld, actor.head);
  }

  outcome::result<uint64_t> PaymentChannelManagerImpl::getNextNonce(
      const ChannelInfo &channel, const LaneId &lane) const {
    auto lookup_lane = channel.lanes.find(lane);
    if (lookup_lane == channel.lanes.end()) {
      return 1;
    }
    Nonce nonce{0};
    for (auto &voucher : lookup_lane->second) {
      if (nonce < voucher.nonce) nonce = voucher.nonce;
    }
    return nonce + 1;
  }

}  // namespace fc::payment_channel_manager
