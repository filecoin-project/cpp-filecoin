/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_RUNTIME_IMPL_RUNTIME_IMPL_HPP
#define CPP_FILECOIN_CORE_VM_RUNTIME_IMPL_RUNTIME_IMPL_HPP

#include "crypto/randomness/randomness_provider.hpp"
#include "storage/ipfs/datastore.hpp"
#include "vm/actor/invoker.hpp"
#include "vm/runtime/actor_state_handle.hpp"
#include "vm/runtime/runtime.hpp"
#include "vm/state/state_tree.hpp"

namespace fc::vm::runtime {

  using actor::Actor;
  using actor::Invoker;
  using crypto::randomness::ChainEpoch;
  using crypto::randomness::Randomness;
  using crypto::randomness::RandomnessProvider;
  using indices::Indices;
  using primitives::address::Address;
  using state::StateTree;
  using storage::ipfs::IpfsDatastore;

  class RuntimeImpl : public Runtime {
   public:
    RuntimeImpl(std::shared_ptr<RandomnessProvider> randomness_provider,
                std::shared_ptr<IpfsDatastore> datastore,
                std::shared_ptr<StateTree> state_tree,
                std::shared_ptr<Indices> indices,
                std::shared_ptr<Invoker> invoker,
                std::shared_ptr<UnsignedMessage> message,
                ChainEpoch chain_epoch,
                Address immediate_caller,
                Address block_miner,
                BigInt gas_available,
                BigInt gas_used);

    /** \copydoc Runtime::getCurrentEpoch() */
    ChainEpoch getCurrentEpoch() const override;

    /** \copydoc Runtime::getRandomness() */
    Randomness getRandomness(DomainSeparationTag tag,
                             ChainEpoch epoch) const override;

    /** \copydoc Runtime::getRandomness() */
    Randomness getRandomness(DomainSeparationTag tag,
                             ChainEpoch epoch,
                             Serialization seed) const override;

    /** \copydoc Runtime::getImmediateCaller() */
    Address getImmediateCaller() const override;

    /** \copydoc Runtime::getCurrentReceiver() */
    Address getCurrentReceiver() const override;

    /** \copydoc Runtime::getTopLevelBlockWinner() */
    Address getTopLevelBlockWinner() const override;

    /** \copydoc Runtime::acquireState() */
    std::shared_ptr<ActorStateHandle> acquireState() const override;

    /** \copydoc Runtime::getBalance() */
    outcome::result<BigInt> getBalance(const Address &address) const override;

    /** \copydoc Runtime::getValueReceived() */
    BigInt getValueReceived() const override;

    /** \copydoc Runtime::getCurrentIndices() */
    std::shared_ptr<Indices> getCurrentIndices() const override;

    /** \copydoc Runtime::getActorCodeID() */
    outcome::result<CodeId> getActorCodeID(
        const Address &address) const override;

    /** \copydoc Runtime::send() */
    fc::outcome::result<InvocationOutput> send(Address to_address,
                                               MethodNumber method_number,
                                               MethodParams params,
                                               BigInt value) override;

    /** \copydoc Runtime::createActor() */
    outcome::result<void> createActor(const Address &address,
                                      const Actor &actor) override;

    /** \copydoc Runtime::deleteActor() */
    outcome::result<void> deleteActor(const Address &address) override;

    /** \copydoc Runtime::getIpfsDatastore() */
    std::shared_ptr<IpfsDatastore> getIpfsDatastore() override;

    /** \copydoc Runtime::getMessage() */
    std::shared_ptr<UnsignedMessage> getMessage() override;

    outcome::result<void> chargeGas(const BigInt &amount) override;

   private:
    outcome::result<void> transfer(Actor &from,
                                   Actor &to,
                                   const BigInt &amount);
    outcome::result<Actor> getOrCreateActor(const Address &address);
    std::shared_ptr<Runtime> createRuntime(
        const std::shared_ptr<UnsignedMessage> &message) const;

   private:
    std::shared_ptr<RandomnessProvider> randomness_provider_;
    std::shared_ptr<IpfsDatastore> datastore_;
    std::shared_ptr<StateTree> state_tree_;
    std::shared_ptr<Indices> indices_;
    std::shared_ptr<Invoker> invoker_;
    std::shared_ptr<UnsignedMessage> message_;
    ChainEpoch chain_epoch_;
    Address immediate_caller_;
    Address block_miner_;
    BigInt gas_price_;
    BigInt gas_available_;
    BigInt gas_used_;
  };

}  // namespace fc::vm::runtime

#endif  // CPP_FILECOIN_CORE_VM_RUNTIME_IMPL_RUNTIME_IMPL_HPP
