/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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

    Type type;
    SignedMessage message;
  };

  struct Mpool : public std::enable_shared_from_this<Mpool> {
    using Subscriber = void(const MpoolUpdate &);

    static std::shared_ptr<Mpool> create(
        const EnvironmentContext &env_context,
        TsBranchPtr ts_main,
        std::shared_ptr<ChainStore> chain_store);
    std::vector<SignedMessage> pending() const;
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
    std::map<Address, std::map<Nonce, SignedMessage>> by_from;
    std::map<CID, Signature> bls_cache;
    boost::signals2::signal<Subscriber> signal;
    mutable std::default_random_engine generator;
    mutable std::normal_distribution<> distribution;
  };

  extern TokenAmount kDefaultMaxFee;
}  // namespace fc::storage::mpool
