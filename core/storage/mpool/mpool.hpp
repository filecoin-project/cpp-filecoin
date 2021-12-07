/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/compute/detail/lru_cache.hpp>
#include <deque>
#include <random>

#include "common/logger.hpp"
#include "fwd.hpp"
#include "primitives/tipset/chain.hpp"
#include "storage/chain/chain_store.hpp"
#include "vm/message/message.hpp"
#include "vm/runtime/env_context.hpp"

namespace fc::storage::mpool {
  using boost::compute::detail::lru_cache;
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

  const TokenAmount kDefaultMaxFee{kFilecoinPrecision * 7 / 1000};
  const BigInt kBaseFeeLowerBoundFactor{10};
  constexpr size_t kResolvedCacheSize{1000};
  constexpr size_t kLocalAddressesCacheSize{1000};
  constexpr std::chrono::milliseconds kRepublishBatchDelay{100};

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
        const std::shared_ptr<ChainStore> &chain_store,
        std::shared_ptr<sync::PubSubGate> pubsub_gate);

    std::vector<SignedMessage> pending() const;

    // https://github.com/filecoin-project/lotus/blob/8f78066d4f3c4981da73e3328716631202c6e614/chain/messagepool/selection.go#L41
    outcome::result<std::vector<SignedMessage>> select(
        const TipsetCPtr &tipset_ptr, double ticket_quality) const;

    /**
     * Returns next nonce for the actor with address
     */
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

    void remove(const Address &from, Nonce nonce);
    outcome::result<void> onHeadChange(const HeadChange &change);
    connection_t subscribe(const std::function<Subscriber> &subscriber) {
      return signal.connect(subscriber);
    }

    /**
     * Tries to resolves addresses at height (current - kChainFinality), so
     * reorg at the height is impossible. If resolving at height (current -
     * kChainFinality) failed, resolves at current height. In the last case,
     * resolved value is not cached.
     *
     * @param address to resolve
     * @return resolved address
     */
    outcome::result<Address> resolveKeyAtFinality(const Address &address) const;

    /**
     * Publish message via gossip.
     * @param message to publish
     * @return error if error happened
     */
    void publish(const SignedMessage &message);

    /**
     * Publish batch of messages via gossip.
     * Actually methods enqueue messages, so they are sent in the
     * publishFromQueue() which is called on timer kRepublishBatchDelay. This
     * delay is here to encourage the pubsub subsystem to process the  messages
     * serially and avoid creating nonce gaps because of concurrent validation.
     *
     * @param messages to publish
     * @return error if error happened
     */
    void publish(const std::vector<SignedMessage> &messages);

    /**
     * Republish all pending messages. Method must be called in timer loop.
     * @return
     */
    outcome::result<void> republishPendingMessages();

    /**
     * Publishes messages from queue that were added with batch publish. It must
     * be called in timer loop with kRepublishBatchDelay.
     */
    void publishFromQueue();

    static TokenAmount getBaseFeeLowerBound(const TokenAmount &base_fee,
                                            const BigInt &factor);

   private:
    // For empty value in lru cache
    struct Empty {};

    /**
     * Resolves address at height
     */
    outcome::result<Address> resolveKeyAtHeight(
        const Address &address,
        const ChainEpoch &height,
        const TsBranchPtr &ts_branch) const;

    EnvironmentContext env_context;
    TsBranchPtr ts_main;
    IpldPtr ipld;
    std::shared_ptr<PubSubGate> pubsub_gate_;
    ChainStore::connection_t head_sub;

    TipsetCPtr head_;
    mutable std::shared_mutex head_mutex_;

    // pending messages, key is a from address
    std::map<Address, std::map<Nonce, SignedMessage>> pending_;
    mutable std::shared_mutex pending_mutex_;

    std::deque<SignedMessage> publishing_;
    std::mutex publishing_mutex_;

    mutable lru_cache<CID, Signature> bls_cache{0};
    mutable std::mutex bls_cache_mutex_;

    boost::signals2::signal<Subscriber> signal;
    mutable std::default_random_engine generator;
    mutable std::normal_distribution<> distribution;

    // cache of resolved addresses
    mutable lru_cache<Address, Address> resolved_cache_{kResolvedCacheSize};
    mutable std::mutex resolved_cache_mutex_;

    lru_cache<Address, Empty> local_addresses_{kLocalAddressesCacheSize};
    mutable std::shared_mutex local_addresses_mutex_;

    common::Logger logger_;
  };
}  // namespace fc::storage::mpool
