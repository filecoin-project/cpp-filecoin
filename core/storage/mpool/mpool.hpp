/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/compute/detail/lru_cache.hpp>
#include <random>

#include "common/logger.hpp"
#include "fwd.hpp"
#include "node/pubsub_gate.hpp"
#include "primitives/tipset/chain.hpp"
#include "storage/chain/chain_store.hpp"
#include "vm/message/message.hpp"
#include "vm/runtime/env_context.hpp"

namespace fc::storage::mpool {
  using crypto::signature::Signature;
  using primitives::BigInt;
  using primitives::Nonce;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::tipset::HeadChange;
  using primitives::tipset::Tipset;
  using primitives::tipset::TipsetCPtr;
  using primitives::tipset::chain::Path;
  using storage::blockchain::ChainStore;
  using sync::PubSubGate;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;
  using vm::runtime::EnvironmentContext;
  using connection_t = boost::signals2::connection;

  const TokenAmount kDefaultMaxFee = kFilecoinPrecision * 7 / 1000;
  const BigInt kBaseFeeLowerBoundFactor = 10;

  struct MpoolUpdate {
    enum class Type : int64_t { ADD, REMOVE };

    Type type{};
    SignedMessage message;
  };

  struct MessagePool : public std::enable_shared_from_this<MessagePool> {
    using Subscriber = void(const MpoolUpdate &);

    static std::shared_ptr<MessagePool> create(
        const EnvironmentContext &env_context,
        TsBranchPtr ts_main,
        size_t bls_cache_size,
        std::shared_ptr<ChainStore> chain_store,
        std::shared_ptr<sync::PubSubGate> pubsub_gate);
    std::vector<SignedMessage> pending() const;
    // https://github.com/filecoin-project/lotus/blob/8f78066d4f3c4981da73e3328716631202c6e614/chain/messagepool/selection.go#L41
    outcome::result<std::vector<SignedMessage>> select(
        const TipsetCPtr &tipset_ptr, double ticket_quality) const;
    outcome::result<Nonce> nonce(const Address &from) const;
    outcome::result<void> estimate(UnsignedMessage &message,
                                   const TokenAmount &max_fee) const;
    TokenAmount estimateFeeCap(const TokenAmount &premium,
                               int64_t max_blocks) const;
    outcome::result<TokenAmount> estimateGasPremium(int64_t max_blocks) const;

    /**
     * Adds message from 'local' address. It is an message send from the current
     * node API. Such messages are republished.
     * @param message to add
     * @return error if error happened
     */
    outcome::result<void> addLocal(const SignedMessage &message);

    /**
     * Adds message.
     * N.B. Local addresses not updated with this methos, if message is going to
     * be add from the current node API, use `addLocal`.
     * @param message to add
     * @return error if error happened
     */
    outcome::result<void> add(const SignedMessage &message);
    outcome::result<void> addLocked(const SignedMessage &message);

    void remove(const Address &from, Nonce nonce);
    outcome::result<void> onHeadChange(const HeadChange &change);
    connection_t subscribe(const std::function<Subscriber> &subscriber) {
      return signal.connect(subscriber);
    }

    /**
     * Resolves addresses at height (current - kChainFinality), so reorg at the
     * height is impossible.
     * @param address to resolve
     * @return resolved address
     */
    outcome::result<Address> resolveKeyAtFinality(const Address &address) const;

    /**
     * Publish message via gossip.
     * @param message to publish
     * @return error if error happened
     */
    outcome::result<void> publish(const SignedMessage &message);

    /**
     * Publish batch of messages via gossip.
     * @param messages to publish
     * @return error if error happened
     */
    outcome::result<void> publish(const std::vector<SignedMessage> &messages);

    outcome::result<void> republishPendingMessages();

    static TokenAmount getBaseFeeLowerBound(const TokenAmount &base_fee,
                                            const BigInt &factor);

   private:
    EnvironmentContext env_context;
    TsBranchPtr ts_main;
    IpldPtr ipld;
    std::shared_ptr<PubSubGate> pubsub_gate_;
    ChainStore::connection_t head_sub;

    TipsetCPtr head_;
    mutable std::shared_mutex head_mutex_;

    // pending messages, key is a from address
    // TODO(turuslan): FIL-420 check cache memory usage
    std::map<Address, std::map<Nonce, SignedMessage>> pending_;
    mutable std::shared_mutex pending_mutex_;

    mutable boost::compute::detail::lru_cache<CID, Signature> bls_cache{0};
    boost::signals2::signal<Subscriber> signal;
    mutable std::default_random_engine generator;
    mutable std::normal_distribution<> distribution;
    // cache of resolved addresses
    mutable std::map<Address, Address> resolved_cache_;

    mutable std::shared_mutex local_addresses_mutex_;
    std::set<Address> local_addresses_;

    common::Logger logger_;
  };
}  // namespace fc::storage::mpool
