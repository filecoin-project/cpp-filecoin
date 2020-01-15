/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_RUNTIME_IMPL_RUNTIME_IMPL_HPP
#define CPP_FILECOIN_CORE_VM_RUNTIME_IMPL_RUNTIME_IMPL_HPP

#include "crypto/randomness/randomness_provider.hpp"
#include "storage/ipfs/datastore.hpp"
#include "vm/runtime/actor_state_handle.hpp"
#include "vm/runtime/runtime.hpp"
#include "vm/state/state_tree.hpp"

namespace fc::vm::runtime {

  using actor::Actor;
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
                std::shared_ptr<UnsignedMessage> message,
                ChainEpoch chain_epoch,
                Address immediate_caller,
                Address receiver,
                Address block_miner,
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

    //    // TODO(a.chernyshov) implement
    //    /** \copydoc Runtime::returnSuccess() */
    //    InvocationOutput returnSuccess() override;

    //    // TODO(a.chernyshov) implement
    //    /** \copydoc Runtime::returnValue() */
    //    InvocationOutput returnValue(Buffer bytes) override;
    //
    //    // TODO(a.chernyshov) implement
    //    /** \copydoc Runtime::abort() */
    //    void abort(const ExitCode &errExitCode, const std::string &msg)
    //    override;
    //
    //    // TODO(a.chernyshov) implement
    //    void abortArg(const std::string &message) override;
    //    // TODO(a.chernyshov) implement
    //    void abortArg() override;
    //
    //    // TODO(a.chernyshov) implement
    //    void abortState(const std::string &message) override;
    //    // TODO(a.chernyshov) implement
    //    void abortState() override;
    //
    //    // TODO(a.chernyshov) implement
    //    void abortFunds(const std::string &message) override;
    //    // TODO(a.chernyshov) implement
    //    void abortFunds() override;
    //
    //    // TODO(a.chernyshov) implement
    //    void abortAPI(const std::string &message) override;
    //
    //    // TODO(a.chernyshov) implement
    //    // TODO(a.chernyshov) assert
    //    // void assert(bool condition) override;

    outcome::result<BigInt> getBalance(const Address &address) const override;

    //    // TODO(a.chernyshov) implement
    //    BigInt getValueReceived() const override;

    /** \copydoc Runtime::getCurrentIndices() */
    std::shared_ptr<Indices> getCurrentIndices() const override;

    /** \copydoc Runtime::getActorCodeID() */
    outcome::result<CodeId> getActorCodeID(
        const Address &address) const override;

    //    // TODO(a.chernyshov) implement
    //    // TODO(a.chernyshov) util.Any type
    //    // Compute(ComputeFunctionID, args [util.Any]) util.Any
    //
    //    // TODO(a.chernyshov) implement
    //    InvocationOutput sendPropagatingErrors(InvocationInput input)
    //    override;
    /** \copydoc Runtime::send() */
    fc::outcome::result<InvocationOutput> send(Address to_address,
                                               MethodNumber method_number,
                                               MethodParams params,
                                               BigInt value) override;

    //    // TODO(a.chernyshov) implement
    //    Serialization sendQuery(Address to_addr,
    //                                    MethodNumber method_number,
    //                                    gsl::span<Serialization> params)
    //                                    override;
    //    // TODO(a.chernyshov) implement
    //    void sendFunds(const Address &to_address, const BigInt &value)
    //    override;
    //
    //    // TODO(a.chernyshov) implement
    //    std::tuple<InvocationOutput, ExitCode> sendCatchingErrors(
    //        InvocationInput input) override;
    //
    //    // TODO(a.chernyshov) implement
    //    Address createNewActorAddress() override;

    /** \copydoc Runtime::createActor() */
    outcome::result<void> createActor(CodeId code_id,
                                      const Address &address) override;

    /** \copydoc Runtime::deleteActor() */
    outcome::result<void> deleteActor(const Address &address) override;

    /** \copydoc Runtime::getIpfsDatastore() */
    std::shared_ptr<IpfsDatastore> getIpfsDatastore() override;

    /** \copydoc Runtime::getMessage() */
    std::shared_ptr<UnsignedMessage> getMessage() override;

   private:
    outcome::result<Actor> getOrCreateActor(const Address &address);

   private:
    std::shared_ptr<RandomnessProvider> randomness_provider_;
    std::shared_ptr<IpfsDatastore> datastore_;
    std::shared_ptr<StateTree> state_tree_;
    std::shared_ptr<Indices> indices_;
    std::shared_ptr<UnsignedMessage> message_;
    ChainEpoch chain_epoch_;
    Address immediate_caller_;
    Address receiver_;
    Address block_miner_;
    BigInt gas_used_;
  };

}  // namespace fc::vm::runtime

#endif  // CPP_FILECOIN_CORE_VM_RUNTIME_IMPL_RUNTIME_IMPL_HPP
