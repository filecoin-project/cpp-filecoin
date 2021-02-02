/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_VM_RUNTIME_ENV_HPP
#define FILECOIN_CORE_VM_RUNTIME_ENV_HPP

#include "primitives/tipset/tipset.hpp"
#include "primitives/types.hpp"
#include "storage/hamt/hamt.hpp"
#include "vm/actor/invoker.hpp"
#include "vm/runtime/circulating.hpp"
#include "vm/runtime/pricelist.hpp"
#include "vm/runtime/runtime_randomness.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

namespace fc::vm::runtime {
  using actor::Invoker;
  using primitives::tipset::TipsetCPtr;
  using state::StateTree;
  using state::StateTreeImpl;

  outcome::result<Address> resolveKey(StateTree &state_tree,
                                      const Address &address,
                                      bool no_actor = false);

  struct IpldBuffered : public Ipld,
                        public std::enable_shared_from_this<IpldBuffered> {
    IpldBuffered(IpldPtr ipld);
    outcome::result<void> flush(const CID &root);

    outcome::result<bool> contains(const CID &key) const override;
    outcome::result<void> set(const CID &key, Value value) override;
    outcome::result<Value> get(const CID &key) const override;
    outcome::result<void> remove(const CID &key) override;
    IpldPtr shared() override;

    IpldPtr ipld;
    bool flushing{false};
    // vm only stores "DAG_CBOR blake2b_256" cids
    std::unordered_map<Hash256, Buffer> write;
  };

  /// Environment contains objects that are shared by runtime contexts
  struct Env : std::enable_shared_from_this<Env> {
    Env(std::shared_ptr<Invoker> invoker,
        std::shared_ptr<RuntimeRandomness> randomness,
        IpldPtr ipld,
        TsBranchPtr ts_branch,
        TipsetCPtr tipset);

    struct Apply {
      MessageReceipt receipt;
      TokenAmount penalty, reward;
    };

    outcome::result<Apply> applyMessage(const UnsignedMessage &message,
                                        size_t size);

    outcome::result<MessageReceipt> applyImplicitMessage(
        UnsignedMessage message);

    std::shared_ptr<IpldBuffered> ipld;
    std::shared_ptr<StateTreeImpl> state_tree;
    std::shared_ptr<Invoker> invoker;
    std::shared_ptr<RuntimeRandomness> randomness;
    uint64_t epoch;  // mutable epoch for cron()
    TsBranchPtr ts_branch;
    TipsetCPtr tipset;
    Pricelist pricelist;
    std::shared_ptr<Circulating> circulating;
  };

  struct Execution : std::enable_shared_from_this<Execution> {
    static std::shared_ptr<Execution> make(std::shared_ptr<Env> env,
                                           const UnsignedMessage &message);

    outcome::result<void> chargeGas(GasAmount amount);

    outcome::result<Actor> tryCreateAccountActor(const Address &address);

    outcome::result<InvocationOutput> sendWithRevert(
        const UnsignedMessage &message);

    outcome::result<InvocationOutput> send(const UnsignedMessage &message,
                                           GasAmount charge = 0);

    std::shared_ptr<Env> env;
    std::shared_ptr<StateTreeImpl> state_tree;
    IpldPtr charging_ipld;
    GasAmount gas_used;
    GasAmount gas_limit;
    Address origin;
    uint64_t origin_nonce;
    size_t actors_created{};
  };

  struct ChargingIpld : public Ipld,
                        public std::enable_shared_from_this<ChargingIpld> {
    ChargingIpld(std::weak_ptr<Execution> execution) : execution_{execution} {}
    outcome::result<bool> contains(const CID &key) const override {
      throw "not implemented";
    }
    outcome::result<void> set(const CID &key, Value value) override;
    outcome::result<Value> get(const CID &key) const override;
    outcome::result<void> remove(const CID &key) override {
      throw "deprecated";
    }
    IpldPtr shared() override {
      return shared_from_this();
    }

    std::weak_ptr<Execution> execution_;
  };
}  // namespace fc::vm::runtime

#endif  // FILECOIN_CORE_VM_RUNTIME_ENV_HPP
