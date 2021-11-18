/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "fwd.hpp"
#include "storage/map_prefix/prefix.hpp"
#include "vm/actor/builtin/states/payment_channel/payment_channel_actor_state.hpp"

#include <mutex>

namespace fc::paych_vouchers {
  using ApiPtr = std::shared_ptr<api::FullNodeApi>;
  using primitives::Nonce;
  using primitives::TokenAmount;
  using primitives::address::ActorExecHash;
  using primitives::address::Address;
  using storage::MapPtr;
  using vm::actor::builtin::states::PaymentChannelActorStatePtr;
  using vm::actor::builtin::types::payment_channel::LaneId;
  using vm::actor::builtin::types::payment_channel::LaneState;
  using vm::actor::builtin::types::payment_channel::SignedVoucher;

  outcome::result<ActorExecHash> actorHash(const Address &paych);

  struct LaneVouchers {
    LaneId lane{};
    // note: `check()` and `add()` ignore vouchers already added by `make()`
    boost::optional<Nonce> added_nonce{};
    // note: ordered by nonce
    std::vector<SignedVoucher> vouchers{};
  };
  CBOR_TUPLE(LaneVouchers, lane, added_nonce, vouchers)

  struct Row {
    static Bytes key(ActorExecHash paych);

    LaneId next_lane{};
    // note: ordered by lane id
    std::vector<LaneVouchers> lanes;
  };
  CBOR_TUPLE(Row, next_lane, lanes)

  using Lanes = std::map<LaneId, LaneState>;

  struct Ctx {
    storage::OneKey key;
    Row row{};

    // note: actor state
    TokenAmount balance{};
    PaymentChannelActorStatePtr state{};

    // note: lane states combined from actor state and vouchers
    bool accepting{};
    boost::optional<Lanes> lanes{};
    boost::optional<TokenAmount> total{};
  };

  struct PaychVouchers {
    PaychVouchers(const IpldPtr &ipld, const ApiPtr &api, const MapPtr &kv);

    outcome::result<LaneId> nextLane(const ActorExecHash &paych);
    outcome::result<void> check(const SignedVoucher &voucher);
    outcome::result<TokenAmount> add(const SignedVoucher &voucher,
                                     const TokenAmount &min_delta);
    outcome::result<SignedVoucher> make(const ActorExecHash &paych,
                                        LaneId lane,
                                        const TokenAmount &amount);

    outcome::result<Ctx> loadCtx(const ActorExecHash &paych);
    outcome::result<TokenAmount> check(Ctx &ctx, const SignedVoucher &voucher);

    IpldPtr ipld;
    ApiPtr api;
    MapPtr kv;
    std::mutex mutex;
  };
}  // namespace fc::paych_vouchers
