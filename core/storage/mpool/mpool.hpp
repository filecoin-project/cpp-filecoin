/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/compute/detail/lru_cache.hpp>
#include <random>

#include "fwd.hpp"
#include "primitives/tipset/chain.hpp"
#include "storage/chain/chain_store.hpp"
#include "vm/message/message.hpp"
#include "vm/runtime/env_context.hpp"

namespace fc::storage::mpool {
  using crypto::signature::Signature;
  using primitives::Nonce;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::tipset::HeadChange;
  using primitives::tipset::Tipset;
  using primitives::tipset::chain::Path;
  using storage::blockchain::ChainStore;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;
  using connection_t = boost::signals2::connection;
  using primitives::tipset::TipsetCPtr;
  using vm::runtime::EnvironmentContext;

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
        std::shared_ptr<ChainStore> chain_store);
    std::vector<SignedMessage> pending() const;
    // https://github.com/filecoin-project/lotus/blob/8f78066d4f3c4981da73e3328716631202c6e614/chain/messagepool/selection.go#L41
    outcome::result<std::vector<SignedMessage>> select(
        TipsetCPtr ts, double ticket_quality) const;
    outcome::result<Nonce> nonce(const Address &from) const;
    outcome::result<void> estimate(UnsignedMessage &message,
                                   const TokenAmount &max_fee) const;
    TokenAmount estimateFeeCap(const TokenAmount &premium,
                               int64_t max_blocks) const;
    outcome::result<TokenAmount> estimateGasPremium(int64_t max_blocks) const;
    outcome::result<void> add(const SignedMessage &message);
    void remove(const Address &from, Nonce nonce);
    outcome::result<void> onHeadChange(const HeadChange &change);
    connection_t subscribe(const std::function<Subscriber> &subscriber) {
      return signal.connect(subscriber);
    }

   private:
    EnvironmentContext env_context;
    TsBranchPtr ts_main;
    IpldPtr ipld;
    ChainStore::connection_t head_sub;
    TipsetCPtr head;
    // TODO(turuslan): FIL-420 check cache memory usage
    std::map<Address, std::map<Nonce, SignedMessage>> by_from;
    mutable boost::compute::detail::lru_cache<CID, Signature> bls_cache{0};
    boost::signals2::signal<Subscriber> signal;
    mutable std::default_random_engine generator;
    mutable std::normal_distribution<> distribution;
  };

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern TokenAmount kDefaultMaxFee;
}  // namespace fc::storage::mpool
