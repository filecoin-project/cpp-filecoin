/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/mpool/mpool.hpp"
#include "common/logger.hpp"
#include "const.hpp"
#include "primitives/tipset/chain.hpp"
#include "vm/actor/builtin/v0/payment_channel/payment_channel_actor.hpp"
#include "vm/interpreter/interpreter.hpp"
#include "vm/runtime/env.hpp"
#include "vm/state/impl/state_tree_impl.hpp"
#include "vm/toolchain/toolchain.hpp"

namespace fc::storage::mpool {
  using primitives::block::MsgMeta;
  using primitives::tipset::HeadChangeType;
  using vm::message::UnsignedMessage;

  std::shared_ptr<Mpool> Mpool::create(
      const EnvironmentContext &env_context,
      TsBranchPtr ts_main,
      std::shared_ptr<ChainStore> chain_store) {
    auto mpool{std::make_shared<Mpool>()};
    mpool->env_context = env_context;
    mpool->ts_main = std::move(ts_main);
    mpool->ipld = env_context.ipld;
    mpool->head_sub = chain_store->subscribeHeadChanges([=](auto &change) {
      auto res{mpool->onHeadChange(change)};
      if (!res) {
        spdlog::error("Mpool.onHeadChange: error {} \"{}\"",
                      res.error(),
                      res.error().message());
      }
    });
    return mpool;
  }

  std::vector<SignedMessage> Mpool::pending() const {
    std::vector<SignedMessage> messages;
    for (auto &[addr, pending] : by_from) {
      for (auto &[nonce, message] : pending.by_nonce) {
        messages.push_back(message);
      }
    }
    return messages;
  }

  outcome::result<uint64_t> Mpool::nonce(const Address &from) const {
    OUTCOME_TRY(interpeted, env_context.interpreter_cache->get(head->key));
    OUTCOME_TRY(
        actor, vm::state::StateTreeImpl{ipld, interpeted.state_root}.get(from));
    auto by_from_it{by_from.find(from)};
    if (by_from_it != by_from.end() && by_from_it->second.nonce > actor.nonce) {
      return by_from_it->second.nonce;
    }
    return actor.nonce;
  }

  outcome::result<void> Mpool::estimate(UnsignedMessage &message) const {
    assert(message.from.isKeyType());
    if (message.gas_limit == 0) {
      auto msg{message};
      msg.gas_limit = kBlockGasLimit;
      msg.gas_fee_cap = kMinimumBaseFee + 1;
      msg.gas_premium = 1;
      OUTCOME_TRY(interpeted, env_context.interpreter_cache->get(head->key));
      auto env{std::make_shared<vm::runtime::Env>(env_context, ts_main, head)};
      env->state_tree = std::make_shared<vm::state::StateTreeImpl>(
          ipld, interpeted.state_root);
      ++env->epoch;
      auto _pending{by_from.find(msg.from)};
      if (_pending != by_from.end()) {
        for (auto &_msg : _pending->second.by_nonce) {
          auto &msg{_msg.second};
          OUTCOME_TRY(env->applyMessage(msg.message, msg.chainSize()));
        }
      }
      OUTCOME_TRY(actor, env->state_tree->get(msg.from));
      msg.nonce = actor.nonce;
      OUTCOME_TRY(
          apply,
          env->applyMessage(
              msg,
              msg.from.isBls()
                  ? msg.chainSize()
                  : SignedMessage{msg, crypto::signature::Secp256k1Signature{}}
                        .chainSize()));
      if (apply.receipt.exit_code != vm::VMExitCode::kOk) {
        return apply.receipt.exit_code;
      }
      if (msg.method
          == vm::actor::builtin::v0::payment_channel::Collect::Number) {
        auto matcher{vm::toolchain::Toolchain::createAddressMatcher(
            vm::version::getNetworkVersion(head->height()))};
        if (matcher->isPaymentChannelActor(actor.code)) {
          apply.receipt.gas_used += 76000;
        }
      }
      message.gas_limit = apply.receipt.gas_used * kGasLimitOverestimation;
    }
    // TODO: premium
    // TODO: fee cap
    return outcome::success();
  }

  outcome::result<void> Mpool::add(const SignedMessage &message) {
    if (message.signature.isBls()) {
      bls_cache.emplace(message.getCid(), message.signature);
    }
    OUTCOME_TRY(ipld->setCbor(message));
    OUTCOME_TRY(ipld->setCbor(message.message));
    auto &pending{by_from[message.message.from]};
    if (pending.by_nonce.empty() || message.message.nonce >= pending.nonce) {
      pending.nonce = message.message.nonce + 1;
    }
    pending.by_nonce[message.message.nonce] = message;
    signal({MpoolUpdate::Type::ADD, message});
    return outcome::success();
  }

  void Mpool::remove(const Address &from, uint64_t nonce) {
    auto by_from_it{by_from.find(from)};
    if (by_from_it != by_from.end()) {
      auto &pending{by_from_it->second};
      auto message{pending.by_nonce.find(nonce)};
      if (message != pending.by_nonce.end()) {
        signal({MpoolUpdate::Type::REMOVE, message->second});
        pending.by_nonce.erase(message);
        if (pending.by_nonce.empty()) {
          by_from.erase(by_from_it);
        } else {
          pending.nonce = std::max(pending.by_nonce.rbegin()->first, nonce) + 1;
        }
      }
    }
  }

  outcome::result<void> Mpool::onHeadChange(const HeadChange &change) {
    if (change.type == HeadChangeType::CURRENT) {
      head = change.value;
    } else {
      auto apply{change.type == HeadChangeType::APPLY};
      OUTCOME_TRY(change.value->visitMessages(
          {ipld, false, true},
          [&](auto, auto bls, auto &cid, auto *smsg, auto *msg)
              -> outcome::result<void> {
            if (apply) {
              remove(msg->from, msg->nonce);
            } else {
              if (bls) {
                auto sig{bls_cache.find(cid)};
                if (sig != bls_cache.end()) {
                  OUTCOME_TRY(add({*msg, sig->second}));
                }
              } else {
                OUTCOME_TRY(add(*smsg));
              }
            }
            return outcome::success();
          }));
      if (apply) {
        head = change.value;
      } else {
        OUTCOME_TRYA(head,
                     env_context.ts_load->load(change.value->getParents()));
      }
    }
    return outcome::success();
  }
}  // namespace fc::storage::mpool
