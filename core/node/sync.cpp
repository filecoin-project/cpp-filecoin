/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/peer/peer_info.hpp>

#include "blockchain/impl/weight_calculator_impl.hpp"
#include "node/blocksync.hpp"
#include "node/sync.hpp"
#include "storage/chain/chain_store.hpp"
#include "vm/interpreter/interpreter.hpp"

#define MOVE(x)  \
  x {            \
    std::move(x) \
  }

namespace fc::sync {
  using primitives::block::MsgMeta;
  using primitives::tipset::Tipset;

  TsSync::TsSync(std::shared_ptr<Host> host,
                 IpldPtr ipld,
                 std::shared_ptr<Interpreter> interpreter)
      : MOVE(host), MOVE(ipld), MOVE(interpreter) {}

  void TsSync::sync(const TipsetKey &key,
                    const PeerId &peer,
                    Callback callback) {
    auto _valid{valid.find(key)};
    if (_valid != valid.end()) {
      return callback(key, _valid->second);
    }
    callbacks[key].push_back(std::move(callback));
    walkDown(key, peer);
  }

  void TsSync::walkDown(TipsetKey key, const PeerId &peer) {
    while (true) {
      auto _ts{Tipset::load(*ipld, key.cids)};
      if (_ts) {
        auto &ts{_ts.value()};
        auto have_messages{true};
        for (auto &block : ts.blks) {
          auto _have{ipld->contains(block.messages)};
          if (!_have || !_have.value()) {
            have_messages = false;
            break;
          }
        }
        if (have_messages) {
          auto parent{ts.getParents()};
          children[parent].push_back(std::move(key));
          if (children.at(parent).size() != 1) {
            return;
          }
          if (valid.find(parent) != valid.end()) {
            return walkUp(std::move(parent));
          }
          key = std::move(parent);
          continue;
        }
      }
      return blocksync::fetch(host,
                              {peer, {}},
                              ipld,
                              key.cids,
                              [self{shared_from_this()}, key, peer](auto _ts) {
                                // TODO: bad block vs network failure
                                if (_ts) {
                                  self->walkDown(std::move(key), peer);
                                }
                              });
    }
  }

  void TsSync::walkUp(TipsetKey key) {
    std::vector<TipsetKey> queue{key};
    while (!queue.empty()) {
      key = std::move(queue.back());
      queue.pop_back();
      auto _valid{valid.at(key)};
      auto _callbacks{callbacks.find(key)};
      if (_callbacks != callbacks.end()) {
        for (auto &callback : _callbacks->second) {
          callback(key, _valid);
        }
        callbacks.erase(_callbacks);
      }
      auto _children{children.find(key)};
      if (_children != children.end()) {
        if (_valid) {
          OUTCOME_EXCEPT(ts, Tipset::load(*ipld, key.cids));
          OUTCOME_EXCEPT(
              weight,
              blockchain::weight::WeightCalculatorImpl{ipld}.calculateWeight(
                  ts));
          OUTCOME_EXCEPT(vm, interpreter->interpret(ipld, ts));
          for (auto &_child : _children->second) {
            OUTCOME_EXCEPT(child, Tipset::load(*ipld, _child.cids));
            valid.emplace(
                _child,
                child.getParentStateRoot() == vm.state_root
                    && child.getParentMessageReceipts() == vm.message_receipts
                    && child.getParentWeight() == weight);
          }
        } else {
          for (auto &child : _children->second) {
            valid.emplace(child, false);
          }
        }
        for (auto &child : _children->second) {
          queue.push_back(std::move(child));
        }
        children.erase(_children);
      }
    }
  }

  Sync::Sync(IpldPtr ipld,
             std::shared_ptr<TsSync> ts_sync,
             std::shared_ptr<ChainStore> chain_store)
      : MOVE(ipld), MOVE(ts_sync), MOVE(chain_store) {
    this->ts_sync->valid.emplace(TipsetKey{this->chain_store->genesisCid()},
                                 true);
  }

  void Sync::onHello(const TipsetKey &key, const PeerId &peer) {
    ts_sync->sync(key, peer, [self{shared_from_this()}](auto &key, auto valid) {
      if (valid) {
        OUTCOME_EXCEPT(ts, Tipset::load(*self->ipld, key.cids));
        std::ignore = self->chain_store->updateHeaviestTipset(ts);
      }
    });
  }

  outcome::result<void> Sync::onGossip(const BlockWithCids &block,
                                       const PeerId &peer) {
    OUTCOME_TRY(have_messages, ipld->contains(block.header.messages));
    if (!have_messages) {
      MsgMeta messages;
      ipld->load(messages);
      auto collect_messages{
          [&](auto &cids, auto &array) -> outcome::result<bool> {
            for (auto &cid : cids) {
              OUTCOME_TRY(array.append(cid));
              OUTCOME_TRY(have, ipld->contains(cid));
              if (!have) {
                return false;
              }
            }
            return true;
          }};
      OUTCOME_TRY(have_bls,
                  collect_messages(block.bls_messages, messages.bls_messages));
      OUTCOME_TRY(
          have_secp,
          collect_messages(block.secp_messages, messages.secp_messages));
      if (have_bls && have_secp) {
        OUTCOME_TRY(messages_cid, ipld->setCbor(messages));
        if (messages_cid != block.header.messages) {
          return blocksync::Error::kInconsistent;
        }
      }
    }
    OUTCOME_TRY(cid, ts_sync->ipld->setCbor(block.header));
    ts_sync->sync({cid},
                  peer,
                  [self{shared_from_this()}, block{std::move(block.header)}](
                      auto &, auto valid) {
                    if (valid) {
                      std::ignore = self->chain_store->addBlock(block);
                    }
                  });
    return outcome::success();
  }
}  // namespace fc::sync
