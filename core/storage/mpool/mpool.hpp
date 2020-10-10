/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_MPOOL_MPOOL_HPP
#define CPP_FILECOIN_CORE_STORAGE_MPOOL_MPOOL_HPP

#include "node/fwd.hpp"
#include "storage/chain/chain_store.hpp"
#include "vm/message/message.hpp"

namespace fc::storage::mpool {
  using crypto::signature::Signature;
  using primitives::address::Address;
  using primitives::tipset::HeadChange;
  using primitives::tipset::Tipset;
  using storage::blockchain::ChainStore;
  using vm::interpreter::Interpreter;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;
  using connection_t = boost::signals2::connection;

  struct MpoolUpdate {
    enum class Type : int64_t { ADD, REMOVE };

    Type type;
    SignedMessage message;
  };

  struct Mpool : public std::enable_shared_from_this<Mpool> {
    struct Pending {
      std::map<uint64_t, SignedMessage> by_nonce;
      uint64_t nonce;
    };
    using Subscriber = void(const MpoolUpdate &);

    static std::shared_ptr<Mpool> create(
        IpldPtr ipld,
        std::shared_ptr<Interpreter> interpreter,
        std::shared_ptr<ChainStore> chain_store);
    std::vector<SignedMessage> pending() const;
    outcome::result<uint64_t> nonce(const Address &from) const;
    outcome::result<void> estimate(UnsignedMessage &message) const;
    outcome::result<void> add(const SignedMessage &message);
    void remove(const Address &from, uint64_t nonce);
    outcome::result<void> onHeadChange(const HeadChange &change);
    connection_t subscribe(const std::function<Subscriber> &subscriber) {
      return signal.connect(subscriber);
    }

   private:
    IpldPtr ipld;
    std::shared_ptr<Interpreter> interpreter;
    ChainStore::connection_t head_sub;
    Tipset head;
    std::map<Address, Pending> by_from;
    std::map<CID, Signature> bls_cache;
    boost::signals2::signal<Subscriber> signal;
  };
}  // namespace fc::storage::mpool

#endif  // CPP_FILECOIN_CORE_STORAGE_MPOOL_MPOOL_HPP
