/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/mpool/mpool.hpp"

#include <boost/math/distributions/binomial.hpp>
#include <chrono>
#include <cmath>
#include <thread>

#include "cbor_blake/ipld_version.hpp"
#include "common/append.hpp"
#include "common/logger.hpp"
#include "common/outcome_fmt.hpp"
#include "common/ptr.hpp"
#include "const.hpp"
#include "node/pubsub_gate.hpp"
#include "vm/actor/builtin/methods/payment_channel.hpp"
#include "vm/interpreter/interpreter.hpp"
#include "vm/runtime/make_vm.hpp"
#include "vm/state/impl/state_tree_impl.hpp"
#include "vm/state/resolve_key.hpp"
#include "vm/toolchain/toolchain.hpp"

namespace fc::storage::mpool {
  using primitives::GasAmount;
  using primitives::block::MsgMeta;
  using primitives::tipset::HeadChangeType;
  using vm::actor::builtin::types::miner::kChainFinality;
  using vm::interpreter::InterpreterCache;
  using vm::message::UnsignedMessage;
  using vm::state::StateTreeImpl;
  namespace paych = vm::actor::builtin::paych;

  constexpr GasAmount kMinGas{1298450};
  constexpr size_t kMaxBlocks{15};
  constexpr size_t kMaxBlockMessages{16000};
  constexpr size_t kRepubMessageLimit{30};

  // https://github.com/filecoin-project/lotus/blob/8f78066d4f3c4981da73e3328716631202c6e614/chain/messagepool/selection.go#L677
  TokenAmount getGasReward(const UnsignedMessage &msg,
                           const TokenAmount &base_fee) {
    TokenAmount reward{
        std::min<TokenAmount>(msg.gas_fee_cap - base_fee, msg.gas_premium)
        * msg.gas_limit};
    if (reward < 0) {
      // https://github.com/filecoin-project/lotus/blob/191a05da4872bf9849f178e6db5c0d6e87d05baa/chain/messagepool/selection.go#L687
      reward *= 3;
    }
    return reward;
  }

  // https://github.com/filecoin-project/lotus/blob/8f78066d4f3c4981da73e3328716631202c6e614/chain/messagepool/selection.go#L677
  double getGasPerf(const TokenAmount &reward, GasAmount limit) {
    return boost::multiprecision::cpp_rational{reward * kBlockGasLimit, limit}
        .convert_to<double>();
  }

  // https://github.com/filecoin-project/lotus/blob/8f78066d4f3c4981da73e3328716631202c6e614/chain/messagepool/block_proba.go#L64
  auto blockProbabilities(double ticket_quality) {
    // TODO(turuslan): what to use from boost/math/distributions/poisson.hpp
    // https://github.com/filecoin-project/lotus/blob/8f78066d4f3c4981da73e3328716631202c6e614/chain/messagepool/block_proba.go#L32
    static auto no_winners{[] {
      std::vector<double> out;
      out.resize(kMaxBlocks);
      auto mu{5};
      auto cond{log(-1 + exp(mu))};
      for (auto i{0u}; i < out.size(); ++i) {
        auto x{i + 1.0};
        out[i] = exp(log(mu) * x - lgamma(x + 1) - cond);
      }
      return out;
    }()};
    std::vector<double> out;
    out.resize(kMaxBlocks);
    auto p{1 - std::clamp<double>(ticket_quality, 0, 1)};
    for (auto k{0u}; k < out.size(); ++k) {
      for (auto n{0u}; n < no_winners.size(); ++n) {
        if (k <= n) {
          out[k] += no_winners[n] * pdf(boost::math::binomial(n, p), k);
        }
      }
    }
    return out;
  }

  void add(std::map<Address, std::map<Nonce, SignedMessage>> &by_from,
           const SignedMessage &smsg) {
    assert(smsg.message.from.isKeyType());
    by_from[smsg.message.from][smsg.message.nonce] = smsg;
  }

  auto remove(std::map<Address, std::map<Nonce, SignedMessage>> &by_from,
              const Address &from,
              Nonce nonce) {
    assert(from.isKeyType());
    boost::optional<SignedMessage> smsg;
    auto from_it{by_from.find(from)};
    if (from_it != by_from.end()) {
      auto &by_nonce{from_it->second};
      auto nonce_it{by_nonce.find(nonce)};
      if (nonce_it != by_nonce.end()) {
        smsg.emplace(std::move(nonce_it->second));
        by_nonce.erase(nonce_it);
        if (by_nonce.empty()) {
          by_from.erase(from_it);
        }
      }
    }
    return smsg;
  }

  struct MsgChain {
    using Ptr = std::shared_ptr<MsgChain>;

    std::vector<SignedMessage> msgs;
    TokenAmount gas_reward;
    GasAmount gas_limit{};
    double gas_perf{};
    double eff_perf{};
    double bp{};
    double parent_offset{};
    bool valid{};
    bool merged{};
    std::weak_ptr<MsgChain> prev;
    std::weak_ptr<MsgChain> next;
  };

  auto mustLock(const std::weak_ptr<MsgChain> &weak) {
    auto ptr{weak.lock()};
    assert(weakEmpty(weak) || ptr);
    return ptr;
  }

  bool before(const MsgChain &l, const MsgChain &r) {
    // right and left are intentionally reversed
    return std::tie(r.gas_perf, r.gas_reward)
           < std::tie(l.gas_perf, l.gas_reward);
  }

  bool beforeEffective(const MsgChain &l, const MsgChain &r) {
    return (l.merged && !r.merged) || (l.gas_perf >= 0 && r.gas_perf < 0)
           || (std::tie(r.eff_perf, r.gas_perf, r.gas_reward) < std::tie(
                   l.eff_perf,
                   l.gas_perf,
                   l.gas_reward));  // right and left are intentionally reversed
  }

  template <typename F>
  auto deref(F f) {
    return [f](auto &l, auto &r) { return f(*l, *r); };
  }

  template <typename T, typename F>
  void bubble(std::vector<T> &xs, size_t i, const F &less) {
    for (auto j{i + 1}; j < xs.size() && !less(xs[j - 1], xs[j]); ++j) {
      std::swap(xs[j - 1], xs[j]);
    }
  }

  void setEffPerf(MsgChain &mc) {
    auto eff_perf{mc.gas_perf * mc.bp};
    auto prev{mustLock(mc.prev)};
    if (eff_perf > 0 && prev) {
      auto parent{(eff_perf * static_cast<double>(mc.gas_limit)
                   + prev->eff_perf * static_cast<double>(prev->gas_limit))
                  / static_cast<double>(mc.gas_limit + prev->gas_limit)};
      mc.parent_offset = eff_perf - parent;
      eff_perf = parent;
    }
    mc.eff_perf = eff_perf;
  }

  void invalidate(MsgChain &mc) {
    mc.valid = false;
    mc.msgs.clear();
    auto next{mustLock(mc.next)};
    if (next) {
      invalidate(*next);
      mc.next.reset();
    }
  }

  void trim(MsgChain &mc, GasAmount gas_limit, const TokenAmount &base_fee) {
    while (!mc.msgs.empty() && (mc.gas_limit > gas_limit || mc.gas_perf < 0)) {
      auto &msg{mc.msgs.back().message};
      mc.gas_reward -= getGasReward(msg, base_fee);
      mc.gas_limit -= msg.gas_limit;
      if (mc.gas_limit > 0) {
        mc.gas_perf = getGasPerf(mc.gas_reward, mc.gas_limit);
        if (mc.bp != 0) {
          setEffPerf(mc);
        }
      } else {
        mc.gas_perf = 0;
        mc.eff_perf = 0;
      }
      mc.msgs.pop_back();
    }
    if (mc.msgs.empty()) {
      mc.valid = false;
    }
    auto next{mustLock(mc.next)};
    if (next) {
      invalidate(*next);
      mc.next.reset();
    }
  }

  void setEffectivePerf(MsgChain &mc, double bp) {
    mc.bp = bp;
    setEffPerf(mc);
  }

  void setNullEffectivePerf(MsgChain &mc) {
    mc.eff_perf = std::min<double>(mc.gas_perf, 0);
  }

  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  std::vector<MsgChain::Ptr> createMessageChains(
      const std::map<Nonce, SignedMessage> &pending,
      const TokenAmount &base_fee,
      Nonce actor_nonce,
      TokenAmount actor_balance,
      const vm::runtime::Pricelist &pricelist) {
    auto gas_limit{kBlockGasLimit};
    std::vector<SignedMessage> msgs;
    for (const auto &[nonce, msg] : pending) {
      if (nonce < actor_nonce) {
        continue;
      }
      if (nonce != actor_nonce) {
        break;
      }
      ++actor_nonce;
      if (msg.message.gas_limit < pricelist.onChainMessage(msg.chainSize())) {
        break;
      }
      gas_limit -= msg.message.gas_limit;
      if (gas_limit < 0) {
        break;
      }
      if (actor_balance < msg.message.requiredFunds()) {
        break;
      }
      actor_balance -= msg.message.requiredFunds();
      actor_balance -= msg.message.value;
      msgs.push_back(msg);
    }
    std::vector<MsgChain::Ptr> chains;
    if (msgs.empty()) {
      return chains;
    }
    auto new_chain{[&] {
      auto chain{std::make_shared<MsgChain>()};
      chain->valid = true;
      chains.push_back(chain);
      return chain;
    }};
    auto cur_chain{new_chain()};
    for (auto &msg : msgs) {
      auto reward{getGasReward(msg.message, base_fee)};
      TokenAmount gas_reward{cur_chain->gas_reward + reward};
      const auto chain_gas_limit{cur_chain->gas_limit + msg.message.gas_limit};
      auto gas_perf{getGasPerf(gas_reward, gas_limit)};
      if (!cur_chain->msgs.empty() && gas_perf < cur_chain->gas_perf) {
        cur_chain = new_chain();
        cur_chain->gas_reward = reward;
        cur_chain->gas_limit = msg.message.gas_limit;
        cur_chain->gas_perf =
            getGasPerf(cur_chain->gas_reward, cur_chain->gas_limit);
      } else {
        cur_chain->gas_reward = gas_reward;
        cur_chain->gas_limit = chain_gas_limit;
        cur_chain->gas_perf = gas_perf;
      }
      cur_chain->msgs.push_back(std::move(msg));
    }
    while (true) {
      auto merged{0};
      auto it{std::prev(chains.end())};
      while (it != chains.begin()) {
        auto &cur{*it};
        --it;
        auto &prev{*it};
        if (cur->gas_perf >= prev->gas_perf) {
          append(prev->msgs, cur->msgs);
          prev->gas_reward += cur->gas_reward;
          prev->gas_limit += cur->gas_limit;
          prev->gas_perf = getGasPerf(prev->gas_reward, prev->gas_limit);
          cur->valid = false;
          ++merged;
        }
      }
      if (merged == 0) {
        break;
      }
      chains.erase(std::remove_if(chains.begin(),
                                  chains.end(),
                                  [](auto &chain) { return !chain->valid; }),
                   chains.end());
    }
    MsgChain::Ptr prev;
    for (auto &chain : chains) {
      if (prev) {
        chain->prev = prev;
        prev->next = chain;
      }
      prev = chain;
    }
    return chains;
  }

  auto greedy(std::vector<MsgChain::Ptr> &chains,
              GasAmount &gas_limit,
              const TokenAmount &base_fee) {
    std::vector<SignedMessage> messages;
    std::sort(chains.begin(), chains.end(), deref(before));
    for (size_t i{0}; i < chains.size(); ++i) {
      auto &chain{chains[i]};
      if (!chain->valid) {
        continue;
      }
      if (chain->gas_perf < 0) {
        break;
      }
      if (chain->gas_limit <= gas_limit) {
        gas_limit -= chain->gas_limit;
        append(messages, chain->msgs);
      } else {
        if (gas_limit < kMinGas) {
          break;
        }
        trim(*chain, gas_limit, base_fee);
        if (chain->valid) {
          bubble(chains, i, deref(before));
          --i;
        }
      }
    }
    return messages;
  }

  auto getDeps(MsgChain::Ptr chain, GasAmount gas_limit) {
    while (true) {
      auto prev{mustLock(chain->prev)};
      if (!prev || prev->merged) {
        return std::make_pair(chain, gas_limit);
      }
      gas_limit -= prev->gas_limit;
      chain = prev;
    }
  }

  void setEffectivePerf(std::vector<MsgChain::Ptr> &chains,
                        const std::vector<double> &block_probability) {
    size_t i{};
    for (auto block{0u}; block < block_probability.size(); ++block) {
      auto gas_limit{kBlockGasLimit};
      for (; i < chains.size() && gas_limit >= kMinGas; ++i) {
        gas_limit -= chains[i]->gas_limit;
        setEffectivePerf(*chains[i], block_probability[block]);
      }
    }
    for (; i < chains.size(); ++i) {
      setNullEffectivePerf(*chains[i]);
    }
    std::sort(chains.begin(), chains.end(), deref(beforeEffective));
  }

  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  auto optimal(std::vector<MsgChain::Ptr> &chains,
               GasAmount &gas_limit,
               const TokenAmount &base_fee,
               double ticket_quality) {
    std::vector<SignedMessage> messages;
    std::sort(chains.begin(), chains.end(), deref(before));
    if (chains.empty() || chains[0]->gas_perf < 0) {
      return messages;
    }
    setEffectivePerf(chains, blockProbabilities(ticket_quality));
    for (size_t i{0}; i < chains.size(); ++i) {
      auto &chain{chains[i]};
      if (!chain->valid || chain->merged) {
        continue;
      }
      if (chain->gas_perf < 0) {
        break;
      }
      auto [dep, gas_available]{getDeps(chain, gas_limit)};
      if (gas_available <= 0) {
        invalidate(*chain);
      } else if (chain->gas_limit > gas_available) {
        if (gas_limit < kMinGas) {
          break;
        }
        trim(*chain, gas_available, base_fee);
        if (chain->valid) {
          bubble(chains, i, deref(beforeEffective));
          --i;
        }
      } else {
        while (true) {
          dep->merged = true;
          append(messages, dep->msgs);
          gas_limit -= dep->gas_limit;
          if (dep == chain) {
            break;
          }
          dep = mustLock(dep->next);
        }
        auto next{mustLock(chain->next)};
        if (next && next->eff_perf > 0) {
          next->eff_perf += next->parent_offset;
          for (next = mustLock(next->next); next && next->eff_perf > 0;
               next = mustLock(next->next)) {
            setEffPerf(*next);
          }
          std::stable_sort(chains.begin() + gsl::narrow<ssize_t>(i),
                           chains.end(),
                           deref(beforeEffective));
        }
      }
    }
    return messages;
  }

  auto optimalRandom(std::vector<MsgChain::Ptr> &chains,
                     GasAmount &gas_limit,
                     const TokenAmount &base_fee,
                     std::default_random_engine &generator) {
    std::vector<SignedMessage> messages;
    if (gas_limit >= kMinGas) {
      std::shuffle(chains.begin(), chains.end(), generator);
    }
    for (auto &chain : chains) {
      if (gas_limit < kMinGas) {
        break;
      }
      if (chain->merged || !chain->valid || chain->gas_perf < 0) {
        continue;
      }
      auto [dep, gas_available]{getDeps(chain, gas_limit)};
      if (gas_available <= 0) {
        invalidate(*chain);
      } else if (chain->gas_limit > gas_available) {
        trim(*chain, gas_available, base_fee);
      }
      if (chain->valid) {
        do {
          dep->merged = true;
          append(messages, dep->msgs);
          gas_limit -= dep->gas_limit;
          dep = mustLock(dep->next);
        } while (dep != chain);
      }
    }
    return messages;
  }

  std::shared_ptr<MessagePool> MessagePool::create(
      const EnvironmentContext &env_context,
      TsBranchPtr ts_main,
      size_t bls_cache_size,
      const std::shared_ptr<ChainStore> &chain_store,
      std::shared_ptr<sync::PubSubGate> pubsub_gate) {
    auto mpool{std::make_shared<MessagePool>()};
    mpool->env_context = env_context;
    mpool->ts_main = std::move(ts_main);
    mpool->ipld = env_context.ipld;
    mpool->head_sub =
        chain_store->subscribeHeadChanges([=](const auto &changes) {
          for (const auto &change : changes) {
            auto res{mpool->onHeadChange(change)};
            if (!res) {
              spdlog::error("MessagePool.onHeadChange: error {} \"{}\"",
                            res.error(),
                            res.error().message());
            }
          }
        });
    mpool->bls_cache = {bls_cache_size};
    mpool->pubsub_gate_ = std::move(pubsub_gate);
    mpool->logger_ = common::createLogger("MessagePool");
    return mpool;
  }

  std::vector<SignedMessage> MessagePool::pending() const {
    std::vector<SignedMessage> messages;
    std::shared_lock pending_lock{pending_mutex_};
    for (const auto &[addr, pending] : pending_) {
      for (const auto &[nonce, message] : pending) {
        messages.push_back(message);
      }
    }
    return messages;
  }

  outcome::result<std::pair<std::vector<TipsetCPtr>, std::vector<TipsetCPtr>>>
  findPath(const TsLoadPtr &ts_load,
           TipsetCPtr from,
           TipsetCPtr to,
           size_t depth) {
    std::vector<TipsetCPtr> revert;
    std::vector<TipsetCPtr> apply;
    while (true) {
      if (from->key == to->key) {
        return std::make_pair(std::move(revert), std::move(apply));
      }
      if (apply.size() > depth || revert.size() > depth) {
        return ERROR_TEXT("findPath: too deep");
      }
      if (from->height() < to->height()) {
        apply.push_back(to);
        OUTCOME_TRYA(to, ts_load->load(to->getParents()));
      } else {
        revert.push_back(from);
        OUTCOME_TRYA(from, ts_load->load(from->getParents()));
      }
    }
  }

  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  outcome::result<std::vector<SignedMessage>> MessagePool::select(
      const TipsetCPtr &tipset_ptr, double ticket_quality) const {
    OUTCOME_TRY(base_fee, tipset_ptr->nextBaseFee(env_context.ipld));
    vm::runtime::Pricelist pricelist{tipset_ptr->epoch()};
    OUTCOME_TRY(cached, env_context.interpreter_cache->get(tipset_ptr->key));
    vm::state::StateTreeImpl state_tree{
        withVersion(env_context.ipld, tipset_ptr->height()), cached.state_root};
    std::shared_lock pending_lock{pending_mutex_};
    auto pending{pending_};
    pending_lock.unlock();
    constexpr auto kDepth{20};
    std::shared_lock head_lock(head_mutex_);
    OUTCOME_TRY(path, findPath(env_context.ts_load, head_, tipset_ptr, kDepth));
    head_lock.unlock();
    for (auto &ts : path.first) {
      OUTCOME_TRY(ts->visitMessages(
          {ipld, false, true},
          [&](auto, auto bls, auto &cid, auto *smsg, auto *msg)
              -> outcome::result<void> {
            if (bls) {
              std::lock_guard bls_cache_lock{bls_cache_mutex_};
              if (auto sig{bls_cache.get(cid)}) {
                mpool::add(pending, {*msg, *sig});
              }
            } else {
              mpool::add(pending, *smsg);
            }
            return outcome::success();
          }));
    }
    for (auto &ts : path.second) {
      OUTCOME_TRY(ts->visitMessages(
          {ipld, false, true},
          [&](auto, auto, auto &, auto *, auto *msg) -> outcome::result<void> {
            mpool::remove(pending, msg->from, msg->nonce);
            return outcome::success();
          }));
    }
    std::vector<SignedMessage> messages;
    std::vector<MsgChain::Ptr> chains;
    GasAmount gas_limit{kBlockGasLimit};
    auto createChains{
        [&](auto &chains, auto &from, auto &by_nonce) -> outcome::result<void> {
          OUTCOME_TRY(actor, state_tree.get(from));
          append(
              chains,
              createMessageChains(
                  by_nonce, base_fee, actor.nonce, actor.balance, pricelist));
          return outcome::success();
        }};
    // TODO(turuslan): priority addrs
    std::vector<Address> priority_addrs;
    for (auto &from : priority_addrs) {
      auto it{pending.find(from)};
      if (it != pending.end()) {
        OUTCOME_TRY(createChains(chains, from, it->second));
        pending.erase(it);
      }
    }
    append(messages, greedy(chains, gas_limit, base_fee));
    chains.clear();
    if (gas_limit < kMinGas) {
      return messages;
    }
    for (auto &[from, by_nonce] : pending) {
      OUTCOME_TRY(createChains(chains, from, by_nonce));
    }
    if (ticket_quality > 0.84) {
      append(messages, greedy(chains, gas_limit, base_fee));
    } else {
      append(messages, optimal(chains, gas_limit, base_fee, ticket_quality));
      append(messages, optimalRandom(chains, gas_limit, base_fee, generator));
    }
    messages.resize(std::min(kMaxBlockMessages, messages.size()));
    return messages;
  }

  outcome::result<Nonce> MessagePool::nonce(const Address &from) const {
    assert(from.isKeyType());
    std::shared_lock head_lock(head_mutex_);
    OUTCOME_TRY(interpeted, env_context.interpreter_cache->get(head_->key));
    OUTCOME_TRY(actor,
                vm::state::StateTreeImpl{withVersion(ipld, head_->height()),
                                         interpeted.state_root}
                    .get(from));
    head_lock.unlock();
    std::shared_lock pending_lock{pending_mutex_};
    auto pending_it{pending_.find(from)};
    if (pending_it != pending_.end()) {
      auto next{pending_it->second.rbegin()->first + 1};
      return std::max(actor.nonce, next);
    }
    return actor.nonce;
  }

  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  outcome::result<void> MessagePool::estimate(
      UnsignedMessage &message, const TokenAmount &max_fee) const {
    assert(message.from.isKeyType());
    if (message.gas_limit == 0) {
      auto msg{message};
      msg.gas_limit = kBlockGasLimit;
      msg.gas_fee_cap = kMinimumBaseFee + 1;
      msg.gas_premium = 1;
      std::shared_lock head_lock(head_mutex_);
      const auto height = head_->height();
      OUTCOME_TRY(interpeted, env_context.interpreter_cache->get(head_->key));
      const auto buf_ipld{std::make_shared<vm::IpldBuffered>(ipld)};
      OUTCOME_TRY(env,
                  vm::makeVm(buf_ipld,
                             env_context,
                             ts_main,
                             head_->getParentBaseFee(),
                             interpeted.state_root,
                             head_->epoch() + 1));
      head_lock.unlock();
      std::shared_lock pending_lock{pending_mutex_};
      auto pending_it{pending_.find(msg.from)};
      if (pending_it != pending_.end()) {
        for (const auto &_msg : pending_it->second) {
          OUTCOME_TRY(env->applyMessage(_msg.second.message, msg.chainSize()));
        }
      }
      pending_lock.unlock();
      OUTCOME_TRY(state, env->flush());
      OUTCOME_TRY(actor,
                  vm::state::StateTreeImpl{
                      withVersion(buf_ipld, head_->height()), state}
                      .get(msg.from));
      OUTCOME_TRYA(env,
                   vm::makeVm(buf_ipld,
                              env_context,
                              ts_main,
                              head_->getParentBaseFee(),
                              state,
                              head_->epoch() + 1));
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
      if (msg.method == paych::Collect::Number) {
        auto matcher{vm::toolchain::Toolchain::createAddressMatcher(
            vm::version::getNetworkVersion(height))};
        if (matcher->isPaymentChannelActor(actor.code)) {
          // https://github.com/filecoin-project/lotus/blob/191a05da4872bf9849f178e6db5c0d6e87d05baa/node/impl/full/gas.go#L281
          constexpr GasAmount kGas{76000};
          apply.receipt.gas_used += kGas;
        }
      }
      message.gas_limit = gsl::narrow_cast<GasAmount>(
          gsl::narrow_cast<double>(apply.receipt.gas_used)
          * kGasLimitOverestimation);
    }
    if (message.gas_premium == 0) {
      OUTCOME_TRYA(message.gas_premium, estimateGasPremium(10));
    }
    if (message.gas_fee_cap == 0) {
      message.gas_fee_cap = estimateFeeCap(message.gas_premium, 20);
    }
    capGasFee(message, max_fee);
    return outcome::success();
  }

  TokenAmount MessagePool::estimateFeeCap(const TokenAmount &premium,
                                          int64_t max_blocks) const {
    std::shared_lock head_lock(head_mutex_);
    return bigdiv(
               head_->getParentBaseFee()
                   * gsl::narrow_cast<uint64_t>(
                       pow(1 + 1.0 / kBaseFeeMaxChangeDenom, max_blocks) * 256),
               256)
           + premium;
  }

  outcome::result<TokenAmount> MessagePool::estimateGasPremium(
      int64_t max_blocks) const {
    if (max_blocks == 0) {
      max_blocks = 1;
    }
    size_t blocks{0};
    std::shared_lock head_lock(head_mutex_);
    auto ts{head_};
    head_lock.unlock();
    std::vector<std::pair<TokenAmount, GasAmount>> prices;
    for (auto i{0}; i < 2 * max_blocks; ++i) {
      if (ts->height() == 0) {
        break;
      }
      OUTCOME_TRYA(ts, env_context.ts_load->load(ts->getParents()));
      blocks += ts->blks.size();
      OUTCOME_TRY(ts->visitMessages(
          {ipld, true, true}, [&](auto, auto, auto, auto, auto msg) {
            prices.emplace_back(msg->gas_premium, msg->gas_limit);
            return outcome::success();
          }));
    }

    std::sort(prices.begin(), prices.end(), [](auto &l, auto &r) {
      // second is intentionally reversed
      return std::tie(r.first, l.second) < std::tie(l.first, r.second);
    });
    auto at = static_cast<int64_t>(kBlockGasTarget * blocks / 2);
    TokenAmount premium;
    TokenAmount prev;
    for (auto &[price, limit] : prices) {
      prev = premium;
      premium = price;
      at -= limit;
      if (at < 0) {
        break;
      }
    }
    if (prev != 0) {
      premium = bigdiv(premium + prev, 2);
    }

    static const TokenAmount kMinGasPremium{100000};
    if (premium < kMinGasPremium) {
      if (max_blocks == 1) {
        premium = 2 * kMinGasPremium;
      } else if (max_blocks == 2) {
        premium = kMinGasPremium * 3 / 2;
      } else {
        premium = kMinGasPremium;
      }
    }

    auto kPrecision{uint64_t{1} << 32};
    auto noise{1 + distribution(generator) * 0.005};
    premium = bigdiv(premium
                         * gsl::narrow_cast<uint64_t>(
                             noise * static_cast<double>(kPrecision) + 1),
                     kPrecision);

    return premium;
  }

  outcome::result<void> MessagePool::addLocal(const SignedMessage &message) {
    OUTCOME_TRY(add(message));
    OUTCOME_TRY(resolved_address, resolveKeyAtFinality(message.message.from));
    std::unique_lock local_addresses_lock{local_addresses_mutex_};
    local_addresses_.insert(resolved_address, {});
    return outcome::success();
  }

  outcome::result<void> MessagePool::add(const SignedMessage &message) {
    if (message.signature.isBls()) {
      std::lock_guard bls_cache_lock{bls_cache_mutex_};
      bls_cache.insert(message.getCid(), message.signature);
    }
    OUTCOME_TRY(setCbor(ipld, message));
    OUTCOME_TRY(setCbor(ipld, message.message));
    std::unique_lock pending_lock{pending_mutex_};
    mpool::add(pending_, message);
    signal({MpoolUpdate::Type::ADD, message});
    return outcome::success();
  }

  void MessagePool::remove(const Address &from, Nonce nonce) {
    std::unique_lock pending_lock{pending_mutex_};
    if (auto smsg{mpool::remove(pending_, from, nonce)}) {
      signal({MpoolUpdate::Type::REMOVE, *smsg});
    }
  }

  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  outcome::result<void> MessagePool::onHeadChange(const HeadChange &change) {
    if (change.type == HeadChangeType::CURRENT) {
      std::unique_lock lock(head_mutex_);
      head_ = change.value;
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
                std::unique_lock bls_cache_lock{bls_cache_mutex_};
                if (auto sig{bls_cache.get(cid)}) {
                  bls_cache_lock.unlock();
                  OUTCOME_TRY(add({*msg, *sig}));
                }
              } else {
                OUTCOME_TRY(add(*smsg));
              }
            }
            return outcome::success();
          }));
      std::unique_lock lock(head_mutex_);
      if (apply) {
        head_ = change.value;
      } else {
        OUTCOME_TRYA(head_,
                     env_context.ts_load->load(change.value->getParents()));
      }
    }
    return outcome::success();
  }

  outcome::result<Address> MessagePool::resolveKeyAtHeight(
      const Address &address,
      const ChainEpoch &height,
      const TsBranchPtr &ts_branch) const {
    OUTCOME_TRY(it, find(ts_branch, height));
    OUTCOME_TRY(tipset_ptr, env_context.ts_load->lazyLoad(it.second->second));

    OUTCOME_TRY(tipset, env_context.interpreter_cache->get(tipset_ptr->key));
    vm::state::StateTreeImpl state_tree{
        withVersion(env_context.ipld, tipset_ptr->height()), tipset.state_root};
    return resolveKey(state_tree, ipld, address);
  }

  outcome::result<Address> MessagePool::resolveKeyAtFinality(
      const Address &address) const {
    if (address.isKeyType()) {
      return address;
    }
    if (!address.isId()) {
      return ERROR_TEXT("Cannot resolve actor address to key address.");
    }
    std::unique_lock resloved_cache_lock{resolved_cache_mutex_};
    if (auto resolved{resolved_cache_.get(address)}) {
      return *resolved;
    }
    resloved_cache_lock.unlock();

    std::shared_lock head_lock(head_mutex_);
    ChainEpoch height = head_->height();
    std::unique_lock ts_lock{*env_context.ts_branches_mutex};
    OUTCOME_TRY(ts_branch,
                TsBranch::make(env_context.ts_load, head_->key, ts_main));
    if (height > kChainFinality) {
      const auto maybe_resolved =
          resolveKeyAtHeight(address, height - kChainFinality, ts_branch);
      if (maybe_resolved) {
        std::lock_guard resloved_cache_lock_guard{resolved_cache_mutex_};
        resolved_cache_.insert(address, maybe_resolved.value());
        return maybe_resolved.value();
      }
    }

    // if failed to resolve at height - finality, or there is no enough height
    // for finality, try to resolve on current height
    OUTCOME_TRY(resolved, resolveKeyAtHeight(address, height, ts_branch));
    return std::move(resolved);
  }

  void MessagePool::publish(const SignedMessage &message) {
    pubsub_gate_->publish(message);
  }

  void MessagePool::publish(const std::vector<SignedMessage> &messages) {
    const std::lock_guard lock(publishing_mutex_);
    for (const auto &message : messages) {
      publishing_.push_back(message);
    }
  }

  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  outcome::result<void> MessagePool::republishPendingMessages() {
    std::shared_lock head_lock(head_mutex_);
    OUTCOME_TRY(base_fee, head_->nextBaseFee(ipld));
    const auto base_fee_lower_bound =
        getBaseFeeLowerBound(base_fee, kBaseFeeLowerBoundFactor);

    std::shared_lock pending_lock{pending_mutex_};
    std::shared_lock local_addresses_lock{local_addresses_mutex_};
    const vm::runtime::Pricelist pricelist{head_->epoch()};
    OUTCOME_TRY(cached, env_context.interpreter_cache->get(head_->key));
    vm::state::StateTreeImpl state_tree{
        withVersion(env_context.ipld, head_->height()), cached.state_root};
    head_lock.unlock();
    std::vector<MsgChain::Ptr> chains;
    for (const auto &[from, mset] : pending_) {
      // republish only local pending messages
      if (local_addresses_.contains(from)) {
        // create _next_ message chains from pending messages taking into
        // account base fee lower bound
        OUTCOME_TRY(actor, state_tree.get(from));
        append(chains,
               createMessageChains(mset,
                                   base_fee_lower_bound,
                                   actor.nonce,
                                   actor.balance,
                                   pricelist));
      }
    }
    if (chains.empty()) {
      return outcome::success();
    }

    // sort chains
    std::sort(chains.begin(), chains.end(), deref(before));

    auto gas_limit = kBlockGasLimit;
    std::vector<SignedMessage> messages;
    for (size_t i = 0; i < chains.size(); ++i) {
      const auto &chain{chains[i]};
      if (chain->msgs.size() > kRepubMessageLimit) {
        break;
      }
      // there is not enough gas for any message
      if (gas_limit <= kMinGas) {
        break;
      }
      if (!chain->valid) {
        continue;
      }

      // does it fit in a block?
      if (chain->gas_limit <= gas_limit) {
        // check the baseFee lower bound -- only republish messages that can be
        // included in the chain within the next 20 blocks.
        for (const auto &message : chain->msgs) {
          if (message.message.gas_fee_cap < base_fee_lower_bound) {
            invalidate(*chain);
            break;
          }
          gas_limit -= message.message.gas_limit;
          messages.push_back(message);
        }
        continue;
      }

      // we can't fit the current chain but there is gas to spare trim it and
      // push it down
      trim(*chain, gas_limit, base_fee);
      // reorder the rest chain
      bubble(chains, i, deref(before));
    }

    logger_->debug("Republishing {} messages", messages.size());
    publish(messages);

    return outcome::success();
  }

  void MessagePool::publishFromQueue() {
    const std::lock_guard lock(publishing_mutex_);
    if (!publishing_.empty()) {
      publish(publishing_.front());
      publishing_.pop_front();
    }
  }

  TokenAmount MessagePool::getBaseFeeLowerBound(const TokenAmount &base_fee,
                                                const BigInt &factor) {
    return std::max(bigdiv(base_fee, factor), kMinimumBaseFee);
  }
}  // namespace fc::storage::mpool
