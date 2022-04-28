/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "paych/maker.hpp"

#include "api/full_node/node_api.hpp"
#include "common/outcome_fmt.hpp"
#include "vm/actor/builtin/methods/init.hpp"
#include "vm/actor/builtin/methods/payment_channel.hpp"
#include "vm/toolchain/toolchain.hpp"

namespace fc::paych_maker {
  using vm::message::UnsignedMessage;
  namespace init = vm::actor::builtin::init;
  namespace paych = vm::actor::builtin::paych;

  inline auto &log() {
    static auto log{common::createLogger("PaychMaker")};
    return log;
  }

  inline UnsignedMessage msgCreate(const PaychMaker::It &it,
                                   NetworkVersion network) {
    const auto &[from_to, queue]{*it};
    return {
        vm::actor::kInitAddress,
        from_to.first,
        {},
        *queue.row.waiting_amount,
        {},
        {},
        init::Exec::Number,
        codec::cbor::encode(
            init::Exec::Params{
                vm::toolchain::Toolchain::createAddressMatcher(network)
                    ->getPaymentChannelCodeId(),
                codec::cbor::encode(paych::Construct::Params{
                                        from_to.first,
                                        from_to.second,
                                    })
                    .value(),
            })
            .value(),
    };
  }

  inline UnsignedMessage msgAdd(const PaychMaker::It &it) {
    const auto &[from_to, queue]{*it};
    return {
        *queue.row.actor,
        from_to.first,
        {},
        *queue.row.waiting_amount - queue.row.unused_amount,
        {},
        {},
        vm::actor::kSendMethodNumber,
        {},
    };
  }

  inline void shift(Queue &queue) {
    assert(!queue.row.waiting_cid);
    assert(queue.waiting_cb.empty());
    std::swap(queue.waiting_cb, queue.pending_cb);
    queue.row.waiting_amount = std::move(queue.pending_amount);
    queue.pending_amount = {};
  }

  Bytes Row::key(const FromTo &from_to) {
    Bytes key;
    append(key, encode(from_to.first));
    append(key, encode(from_to.second));
    return key;
  }

  void Queue::save() {
    key.setCbor(row);
  }

  const CID &UnusedCid::get() {
    if (!cid) {
      cid = key.getCbor<CID>();
    }
    return *cid;
  }

  void UnusedCid::set(const CID &new_cid) {
    cid = new_cid;
    key.setCbor(new_cid);
  }

  inline outcome::result<void> checkKey(Address &address, const ApiPtr &api) {
    if (!address.isKeyType()) {
      if (!address.isId()) {
        return ERROR_TEXT("paych from-to must be key address");
      }
      OUTCOME_TRYA(address, api->StateAccountKey(address, {}));
    }
    return outcome::success();
  }

  PaychMaker::PaychMaker(const ApiPtr &api, const MapPtr &kv)
      : api{api}, kv{kv}, unused_cid{{"unused_cid", kv}} {}

  void PaychMaker::make(FromTo from_to, const TokenAmount &amount, Cb cb) {
    OUTCOME_CB1(checkKey(from_to.first, api));
    OUTCOME_CB1(checkKey(from_to.second, api));
    std::unique_lock lock{mutex};
    auto it{map.find(from_to)};
    const auto empty{it == map.end()};
    if (empty) {
      it = map.emplace(from_to, Queue{{Row::key(from_to), kv}}).first;
    }
    auto &queue{it->second};
    queue.pending_amount += amount;
    queue.pending_cb.emplace_back(std::move(cb));
    if (empty) {
      if (queue.key.has()) {
        queue.key.getCbor(queue.row);
        if (const auto &cid{queue.row.waiting_cid}) {
          lock.unlock();
          api->StateWaitMsg([=](auto &&_wait) { onWait(it, std::move(_wait)); },
                            *cid,
                            kMessageConfidence,
                            api::kLookbackNoLimit,
                            true);
          return;
        }
      } else {
        lock.unlock();
        api->StateNetworkVersion(
            [=](auto &&_network) { onNetwork(it, _network); }, {});
        return;
      }
    }
    lock.unlock();
    next(it);
  }

  void PaychMaker::onNetwork(It it, outcome::result<NetworkVersion> _network) {
    std::unique_lock lock{mutex};
    auto &queue{it->second};
    assert(!queue.row.actor);
    assert(!queue.row.waiting_cid);
    if (!_network) {
      log()->error("StateNetworkVersion {:#}", _network.error());
      onError(it, _network.error());
      return;
    }
    const auto &network{_network.value()};
    shift(queue);
    const auto msg{msgCreate(it, network)};
    lock.unlock();
    api->MpoolPushMessage([=](auto &&_smsg) { onPush(it, std::move(_smsg)); },
                          msg,
                          api::kPushNoSpec);
  }

  void PaychMaker::onPush(It it, outcome::result<SignedMessage> _smsg) {
    std::unique_lock lock{mutex};
    auto &queue{it->second};
    assert(!queue.row.waiting_cid);
    if (!_smsg) {
      log()->error("MpoolPushMessage {:#}", _smsg.error());
      queue.row.waiting_amount.reset();
      queue.save();
      onError(it, _smsg.error());
      return;
    }
    const auto &smsg{_smsg.value()};
    const auto cid{smsg.getCid()};
    queue.row.waiting_cid = cid;
    queue.save();
    lock.unlock();
    api->StateWaitMsg([=](auto &&_wait) { onWait(it, std::move(_wait)); },
                      cid,
                      kMessageConfidence,
                      api::kLookbackNoLimit,
                      true);
  }

  void PaychMaker::onWait(It it, outcome::result<MsgWait> _wait) {
    std::unique_lock lock{mutex};
    auto &queue{it->second};
    const auto cid{*queue.row.waiting_cid};
    if (!_wait) {
      log()->error("StateWaitMsg {} {:#}", cid, _wait.error());
      onError(it, _wait.error());
      return;
    }
    const auto &wait{_wait.value()};
    if (wait.receipt.exit_code != vm::VMExitCode::kOk) {
      queue.row.waiting_amount.reset();
      queue.row.waiting_cid.reset();
      queue.save();
      onError(it, wait.receipt.exit_code);
      return;
    }
    if (!queue.row.actor) {
      auto _result{
          codec::cbor::decode<init::Exec::Result>(wait.receipt.return_value)};
      if (!_result) {
        log()->error("onWait result decode {}",
                     common::hex_lower(wait.receipt.return_value));
        onError(it, _result.error());
        return;
      }
      const auto &result{_result.value()};
      queue.row.actor = result.robust_address;
    }
    queue.row.total_amount += *queue.row.waiting_amount;
    if (queue.waiting_cb.empty()) {
      queue.row.unused_amount += *queue.row.waiting_amount;
      log()->info("unused + {} = {}",
                  *queue.row.waiting_amount,
                  queue.row.unused_amount);
    } else if (auto reused{std::min<TokenAmount>(queue.row.unused_amount,
                                                 *queue.row.waiting_amount)}) {
      queue.row.unused_amount -= reused;
      log()->info("unused - {} = {}", reused, queue.row.unused_amount);
    }
    queue.row.waiting_amount.reset();
    queue.row.waiting_cid.reset();
    queue.save();
    unused_cid.set(cid);
    AddChannelInfo result{*queue.row.actor, cid};
    for (const auto &cb : queue.waiting_cb) {
      cb(result);
    }
    queue.waiting_cb.clear();
    lock.unlock();
    next(it);
  }

  // TODO(turuslan): check actor exists and not settled
  void PaychMaker::next(It it) {
    std::unique_lock lock{mutex};
    auto &queue{it->second};
    if (!queue.row.actor || queue.row.waiting_cid || queue.row.waiting_amount
        || queue.pending_cb.empty()) {
      return;
    }
    if (queue.row.unused_amount >= queue.pending_amount) {
      queue.row.unused_amount -= queue.pending_amount;
      queue.save();
      log()->info(
          "unused - {} = {}", queue.pending_amount, queue.row.unused_amount);
      queue.pending_amount = {};
      AddChannelInfo result{*queue.row.actor, unused_cid.get()};
      for (const auto &cb : queue.pending_cb) {
        cb(result);
      }
      queue.pending_cb.clear();
      return;
    }
    shift(queue);
    queue.save();
    const auto msg{msgAdd(it)};
    lock.unlock();
    api->MpoolPushMessage([=](auto &&_smsg) { onPush(it, std::move(_smsg)); },
                          msg,
                          api::kPushNoSpec);
  }

  void PaychMaker::onError(It it, std::error_code ec) {
    auto &queue{it->second};
    for (const auto &cb : queue.waiting_cb) {
      cb(ec);
    }
    queue.waiting_cb.clear();
    queue.pending_amount = {};
    for (const auto &cb : queue.pending_cb) {
      cb(ec);
    }
    queue.pending_cb.clear();
    map.erase(it);
  }
}  // namespace fc::paych_maker
