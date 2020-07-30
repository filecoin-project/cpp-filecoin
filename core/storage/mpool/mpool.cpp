/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/mpool/mpool.hpp"
#include "vm/interpreter/impl/interpreter_impl.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

namespace fc::storage::mpool {
  using primitives::block::MsgMeta;
  using primitives::tipset::HeadChangeType;
  using vm::message::UnsignedMessage;

  Mpool::Mpool(IpldPtr ipld) : ipld{ipld} {}

  std::shared_ptr<Mpool> Mpool::create(
      IpldPtr ipld, std::shared_ptr<ChainStore> chain_store) {
    auto mpool{std::make_shared<Mpool>(ipld)};
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
    OUTCOME_TRY(interpeted,
                vm::interpreter::InterpreterImpl{}.interpret(ipld, head));
    OUTCOME_TRY(
        actor, vm::state::StateTreeImpl{ipld, interpeted.state_root}.get(from));
    auto by_from_it{by_from.find(from)};
    if (by_from_it != by_from.end() && by_from_it->second.nonce > actor.nonce) {
      return by_from_it->second.nonce;
    }
    return actor.nonce;
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
          ipld, [&](auto, auto bls, auto &cid) -> outcome::result<void> {
            if (bls) {
              OUTCOME_TRY(message, ipld->getCbor<UnsignedMessage>(cid));
              if (apply) {
                remove(message.from, message.nonce);
              } else {
                auto sig{bls_cache.find(cid)};
                if (sig != bls_cache.end()) {
                  OUTCOME_TRY(add({message, sig->second}));
                }
              }
            } else {
              OUTCOME_TRY(message, ipld->getCbor<SignedMessage>(cid));
              if (apply) {
                remove(message.message.from, message.message.nonce);
              } else {
                OUTCOME_TRY(add(message));
              }
            }
            return outcome::success();
          }));
      if (apply) {
        head = change.value;
      } else {
        OUTCOME_TRYA(head, change.value->loadParent(*ipld));
      }
    }
    return outcome::success();
  }
}  // namespace fc::storage::mpool
