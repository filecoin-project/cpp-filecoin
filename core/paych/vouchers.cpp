/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "paych/vouchers.hpp"

#include "api/full_node/node_api.hpp"
#include "cbor_blake/ipld_version.hpp"

namespace fc::paych_vouchers {
  inline outcome::result<void> laneStates(Ctx &ctx) {
    if (ctx.lanes) {
      return outcome::success();
    }
    Lanes lanes;
    OUTCOME_TRY(ctx.state->lanes.visit([&](LaneId id, const LaneState &lane) {
      lanes.emplace(id, lane);
      return outcome::success();
    }));
    for (const auto &[lane_id, added_nonce, vouchers] : ctx.row.lanes) {
      const auto begin{vouchers.begin()};
      auto end{vouchers.end()};
      if (ctx.accepting) {
        if (added_nonce) {
          end = std::upper_bound(
              begin, end, *added_nonce, [](Nonce l, const SignedVoucher &r) {
                return l < r.nonce;
              });
        } else {
          end = begin;
        }
      }
      if (begin == end) {
        continue;
      }
      auto lane_it{lanes.find(lane_id)};
      if (lane_it == lanes.end()) {
        lane_it = lanes
                      .emplace(lane_id,
                               LaneState{
                                   begin->amount,
                                   begin->nonce,
                               })
                      .first;
      }
      auto &lane{lane_it->second};
      for (auto voucher{begin}; voucher != end; ++voucher) {
        lane.nonce = std::max(lane.nonce, voucher->nonce);
        lane.redeem = std::max(lane.redeem, voucher->amount);
      }
    }
    TokenAmount total;
    for (const auto &p : lanes) {
      total += p.second.redeem;
    }
    ctx.lanes.emplace(std::move(lanes));
    ctx.total.emplace(std::move(total));
    return outcome::success();
  }

  inline TokenAmount laneRedeem(const Ctx &ctx, LaneId lane) {
    const auto it{ctx.lanes->find(lane)};
    if (it == ctx.lanes->end()) {
      return 0;
    }
    return it->second.redeem;
  }

  inline auto findVoucher(Row &row, const SignedVoucher &voucher) {
    auto lane_it{std::lower_bound(
        row.lanes.begin(),
        row.lanes.end(),
        voucher.lane,
        [](const LaneVouchers &l, LaneId r) { return l.lane < r; })};
    if (lane_it == row.lanes.end() || lane_it->lane != voucher.lane) {
      lane_it = row.lanes.emplace(lane_it, LaneVouchers{voucher.lane});
    }
    const auto voucher_it{std::lower_bound(
        lane_it->vouchers.begin(),
        lane_it->vouchers.end(),
        voucher,
        [](const SignedVoucher &l, const SignedVoucher &r) {
          return std::tie(l.lane, l.nonce) < std::tie(r.lane, r.nonce);
        })};
    const auto found{voucher_it != lane_it->vouchers.end()
                     && voucher_it->lane == voucher.lane
                     && voucher_it->nonce == voucher.nonce};
    return std::make_tuple(lane_it, voucher_it, found);
  }

  outcome::result<ActorExecHash> actorHash(const Address &paych) {
    if (const auto *hash{boost::get<ActorExecHash>(&paych.data)}) {
      return *hash;
    }
    return ERROR_TEXT("paych address must be actor hash");
  }

  Bytes Row::key(ActorExecHash paych) {
    return copy(paych);
  }

  PaychVouchers::PaychVouchers(const IpldPtr &ipld,
                               const ApiPtr &api,
                               const MapPtr &kv)
      : ipld{ipld}, api{api}, kv{kv} {}

  outcome::result<LaneId> PaychVouchers::nextLane(const ActorExecHash &paych) {
    std::unique_lock lock{mutex};
    OUTCOME_TRY(ctx, loadCtx(paych));
    auto lane{ctx.row.next_lane};
    if (!ctx.row.lanes.empty()) {
      lane = std::max(lane, ctx.row.lanes.rbegin()->lane + 1);
    }
    OUTCOME_TRY(has, ctx.state->lanes.has(lane));
    if (has) {
      spdlog::warn("PaychVouchers::nextLane({}) lane {} exists in state",
                   Address{paych},
                   lane);
    }
    ctx.row.next_lane = lane + 1;
    ctx.key.setCbor(ctx.row);
    return lane;
  }

  outcome::result<void> PaychVouchers::check(const SignedVoucher &voucher) {
    OUTCOME_TRY(paych, actorHash(voucher.channel));
    std::unique_lock lock{mutex};
    OUTCOME_TRY(ctx, loadCtx(paych));
    ctx.accepting = true;
    OUTCOME_TRY(delta, check(ctx, voucher));
    if (*ctx.total + delta > ctx.balance) {
      return ERROR_TEXT("PaychVouchers::check insufficient balance");
    }
    return outcome::success();
  }

  outcome::result<TokenAmount> PaychVouchers::add(
      const SignedVoucher &voucher, const TokenAmount &min_delta) {
    OUTCOME_TRY(paych, actorHash(voucher.channel));
    std::unique_lock lock{mutex};
    OUTCOME_TRY(ctx, loadCtx(paych));
    ctx.accepting = true;
    const auto [lane_it, voucher_it, found]{findVoucher(ctx.row, voucher)};
    if (found && lane_it->added_nonce
        && voucher.nonce <= *lane_it->added_nonce) {
      if (voucher != *voucher_it) {
        return ERROR_TEXT("PaychVouchers::add nonce already used");
      }
      spdlog::warn(
          "PaychVouchers::add(actor={} lane={} nonce={}) already added",
          voucher.channel,
          voucher.lane,
          voucher.nonce);
      return 0;
    }
    OUTCOME_TRY(delta, check(ctx, voucher));
    if (delta < min_delta) {
      return ERROR_TEXT("PaychVouchers::add insufficient voucher");
    }
    if (*ctx.total + delta > ctx.balance) {
      return ERROR_TEXT("PaychVouchers::add insufficient balance");
    }
    if (!found) {
      lane_it->vouchers.insert(voucher_it, voucher);
    }
    lane_it->added_nonce = voucher.nonce;
    ctx.key.setCbor(ctx.row);
    return std::move(delta);
  }

  outcome::result<SignedVoucher> PaychVouchers::make(
      const ActorExecHash &paych, LaneId lane, const TokenAmount &amount) {
    std::unique_lock lock{mutex};
    OUTCOME_TRY(ctx, loadCtx(paych));
    OUTCOME_TRY(laneStates(ctx));
    const TokenAmount delta{amount - laneRedeem(ctx, lane)};
    if (delta <= 0) {
      return ERROR_TEXT("PaychVouchers::make voucher adds no value");
    }
    if (*ctx.total + delta > ctx.balance) {
      return ERROR_TEXT("PaychVouchers::make insufficient balance");
    }
    SignedVoucher voucher;
    voucher.channel = {paych};
    voucher.lane = lane;
    if (const auto it{ctx.lanes->find(lane)}; it != ctx.lanes->end()) {
      if (it->second.nonce == UINT64_MAX) {
        return ERROR_TEXT("PaychVouchers::make lane nonce limit");
      }
      voucher.nonce = it->second.nonce + 1;
    }
    voucher.amount = amount;
    OUTCOME_TRY(
        signature,
        api->WalletSign(ctx.state->from, codec::cbor::encode(voucher).value()));
    voucher.signature_bytes.emplace(signature.toBytes());
    const auto [lane_it, voucher_it, found]{findVoucher(ctx.row, voucher)};
    lane_it->vouchers.insert(voucher_it, voucher);
    ctx.key.setCbor(ctx.row);
    return voucher;
  }

  outcome::result<Ctx> PaychVouchers::loadCtx(const ActorExecHash &paych) {
    OUTCOME_TRY(ts, api->ChainHead());
    Ctx ctx{{Row::key(paych), kv}};
    if (ctx.key.has()) {
      ctx.key.getCbor(ctx.row);
    }
    OUTCOME_TRY(actor, api->StateGetActor({paych}, ts->key));
    ctx.balance = actor.balance;
    OUTCOME_TRY(network, api->StateNetworkVersion(ts->key));
    OUTCOME_TRYA(ctx.state,
                 getCbor<PaymentChannelActorStatePtr>(
                     withVersion(ipld, actorVersion(network)), actor.head));
    if (ctx.state->settling_at != 0 && ts->epoch() >= ctx.state->settling_at) {
      return ERROR_TEXT("paych actor was settled");
    }
    return std::move(ctx);
  }

  outcome::result<TokenAmount> PaychVouchers::check(
      Ctx &ctx, const SignedVoucher &voucher) {
    if (voucher.time_lock_min != 0) {
      return ERROR_TEXT("PaychVouchers::check time_lock_min not supported");
    }
    if (voucher.time_lock_max != 0) {
      return ERROR_TEXT("PaychVouchers::check time_lock_max not supported");
    }
    if (!voucher.secret_preimage.empty()) {
      return ERROR_TEXT("PaychVouchers::check secret_preimage not supported");
    }
    if (voucher.extra) {
      return ERROR_TEXT("PaychVouchers::check extra not supported");
    }
    if (!voucher.merges.empty()) {
      return ERROR_TEXT("PaychVouchers::check merges not supported");
    }
    if (!voucher.signature_bytes) {
      return ERROR_TEXT("PaychVouchers::check empty signature");
    }
    OUTCOME_TRY(
        signature,
        crypto::signature::Signature::fromBytes(*voucher.signature_bytes));
    auto signable{voucher};
    signable.signature_bytes.reset();
    OUTCOME_TRY(
        verified,
        api->WalletVerify(
            ctx.state->from, codec::cbor::encode(signable).value(), signature));
    if (!verified) {
      return ERROR_TEXT("PaychVouchers::check invalid signature");
    }
    OUTCOME_TRY(laneStates(ctx));
    if (const auto it{ctx.lanes->find(voucher.lane)}; it != ctx.lanes->end()) {
      if (voucher.nonce <= it->second.nonce) {
        return ERROR_TEXT("PaychVouchers::check nonce too low");
      }
    }
    TokenAmount delta{voucher.amount - laneRedeem(ctx, voucher.lane)};
    if (delta <= 0) {
      return ERROR_TEXT("PaychVouchers::check voucher adds no value");
    }
    return delta;
  }
}  // namespace fc::paych_vouchers
