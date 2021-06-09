/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/common/metrics/instance_count.hpp>

#include "primitives/tipset/tipset.hpp"
#include "primitives/types.hpp"
#include "vm/actor/invoker.hpp"
#include "vm/runtime/circulating.hpp"
#include "vm/runtime/env_context.hpp"
#include "vm/runtime/pricelist.hpp"
#include "vm/runtime/runtime_randomness.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

namespace fc::vm::runtime {
  using actor::Actor;
  using primitives::tipset::TipsetCPtr;
  using state::StateTree;
  using state::StateTreeImpl;

  /**
   * Returns the public key type of address (`BLS`/`SECP256K1`) of an account
   * actor identified by `address`.
   * @param state_tree - state tree
   * @param ipld - regular or charging ipld
   * @param address - account actor address
   * @param allow_actor - is actor type address allowed
   * @return key address
   */
  outcome::result<Address> resolveKey(StateTree &state_tree,
                                      IpldPtr ipld,
                                      const Address &address,
                                      bool allow_actor = true);

  struct IpldBuffered : public Ipld,
                        public std::enable_shared_from_this<IpldBuffered> {
    IpldBuffered(IpldPtr ipld);
    outcome::result<void> flush(const CID &root);

    outcome::result<bool> contains(const CID &key) const override;
    outcome::result<void> set(const CID &key, Value value) override;
    outcome::result<Value> get(const CID &key) const override;
    IpldPtr shared() override;

    IpldPtr ipld;
    // vm only stores "DAG_CBOR blake2b_256" cids
    std::unordered_map<Hash256, Buffer> write;

    LIBP2P_METRICS_INSTANCE_COUNT(fc::vm::runtime::IpldBuffered);
  };

  /// Environment contains objects that are shared by runtime contexts
  struct Env : std::enable_shared_from_this<Env> {
    Env(const EnvironmentContext &env_context,
        TsBranchPtr ts_branch,
        TipsetCPtr tipset);

    struct Apply {
      MessageReceipt receipt;
      TokenAmount penalty;
      TokenAmount reward;
    };

    outcome::result<Apply> applyMessage(const UnsignedMessage &message,
                                        size_t size);

    outcome::result<MessageReceipt> applyImplicitMessage(
        UnsignedMessage message);

    std::shared_ptr<IpldBuffered> ipld;
    std::shared_ptr<StateTreeImpl> state_tree;
    EnvironmentContext env_context;
    uint64_t epoch;  // mutable epoch for cron()
    TsBranchPtr ts_branch;
    TipsetCPtr tipset;
    Pricelist pricelist;

    LIBP2P_METRICS_INSTANCE_COUNT(fc::vm::runtime::Env);
  };

  struct Execution : std::enable_shared_from_this<Execution> {
    static std::shared_ptr<Execution> make(const std::shared_ptr<Env> &env,
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

    LIBP2P_METRICS_INSTANCE_COUNT(fc::vm::runtime::Execution);
  };

  struct ChargingIpld : public Ipld,
                        public std::enable_shared_from_this<ChargingIpld> {
    ChargingIpld(std::weak_ptr<Execution> execution) : execution_{execution} {}
    outcome::result<bool> contains(const CID &key) const override {
      throw "not implemented";
    }
    outcome::result<void> set(const CID &key, Value value) override;
    outcome::result<Value> get(const CID &key) const override;
    IpldPtr shared() override {
      return shared_from_this();
    }

    std::weak_ptr<Execution> execution_;

    LIBP2P_METRICS_INSTANCE_COUNT(fc::vm::runtime::ChargingIpld);
  };
}  // namespace fc::vm::runtime
