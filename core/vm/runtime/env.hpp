/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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
  using primitives::ChainEpoch;
  using primitives::Nonce;
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

  struct IpldBuffered : Ipld {
    explicit IpldBuffered(IpldPtr ipld);
    outcome::result<void> flush(const CID &root);

    outcome::result<bool> contains(const CID &key) const override;
    outcome::result<void> set(const CID &key, Value value) override;
    outcome::result<Value> get(const CID &key) const override;

    IpldPtr ipld;
    // vm only stores "DAG_CBOR blake2b_256" cids
    std::unordered_map<CbCid, Bytes> write;
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

    void setHeight(ChainEpoch height);

    outcome::result<Apply> applyMessage(const UnsignedMessage &message,
                                        size_t size);

    outcome::result<MessageReceipt> applyImplicitMessage(
        UnsignedMessage message);

    std::shared_ptr<IpldBuffered> ipld;
    std::shared_ptr<StateTreeImpl> state_tree;
    EnvironmentContext env_context;
    ChainEpoch epoch;  // mutable epoch for cron()
    TsBranchPtr ts_branch;
    TipsetCPtr tipset;
    Pricelist pricelist;
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
    Nonce origin_nonce;
    size_t actors_created{};
  };

  struct ChargingIpld : Ipld {
    explicit ChargingIpld(const std::shared_ptr<Execution> &execution)
        : execution_{execution} {
      actor_version = execution->env->ipld->actor_version;
    }
    outcome::result<bool> contains(const CID &key) const override {
      return ERROR_TEXT("not implemented");
    }
    outcome::result<void> set(const CID &key, Value value) override;
    outcome::result<Value> get(const CID &key) const override;

    std::weak_ptr<Execution> execution_;
  };
}  // namespace fc::vm::runtime
