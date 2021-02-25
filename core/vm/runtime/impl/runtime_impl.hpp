/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/states/impl/state_manager_impl.hpp"
#include "vm/actor/invoker.hpp"
#include "vm/runtime/env.hpp"
#include "vm/runtime/runtime.hpp"
#include "vm/state/state_tree.hpp"

namespace fc::vm::runtime {
  using actor::Invoker;
  using actor::builtin::states::StateManagerImpl;

  class RuntimeImpl : public Runtime {
   public:
    RuntimeImpl(std::shared_ptr<Execution> execution,
                UnsignedMessage message,
                const Address &caller_id);

    std::shared_ptr<Execution> execution() const override;

    std::shared_ptr<StateManager> stateManager() const override;

    NetworkVersion getNetworkVersion() const override;

    /** \copydoc Runtime::getCurrentEpoch() */
    ChainEpoch getCurrentEpoch() const override;

    /** \copydoc Runtime::getActorVersion() */
    ActorVersion getActorVersion() const override;

    outcome::result<Randomness> getRandomnessFromTickets(
        DomainSeparationTag tag,
        ChainEpoch epoch,
        gsl::span<const uint8_t> seed) const override;

    outcome::result<Randomness> getRandomnessFromBeacon(
        DomainSeparationTag tag,
        ChainEpoch epoch,
        gsl::span<const uint8_t> seed) const override;

    /** \copydoc Runtime::getImmediateCaller() */
    Address getImmediateCaller() const override;

    /** \copydoc Runtime::getCurrentReceiver() */
    Address getCurrentReceiver() const override;

    /** \copydoc Runtime::getBalance() */
    outcome::result<BigInt> getBalance(const Address &address) const override;

    /** \copydoc Runtime::getValueReceived() */
    BigInt getValueReceived() const override;

    /** \copydoc Runtime::getActorCodeID() */
    outcome::result<CodeId> getActorCodeID(
        const Address &address) const override;

    /** \copydoc Runtime::send() */
    outcome::result<InvocationOutput> send(Address to_address,
                                           MethodNumber method_number,
                                           MethodParams params,
                                           BigInt value) override;

    /** \copydoc Runtime::createNewActorAddress() */
    outcome::result<Address> createNewActorAddress() override;

    /** \copydoc Runtime::createActor() */
    outcome::result<void> createActor(const Address &address,
                                      const Actor &actor) override;

    /** \copydoc Runtime::deleteActor() */
    outcome::result<void> deleteActor(const Address &address) override;

    /** \copydoc Runtime::transfer() */
    outcome::result<void> transfer(const Address &debitFrom,
                                   const Address &creditTo,
                                   const TokenAmount &amount) override;

    /** \copydoc Runtime::getTotalFilCirculationSupply() */
    fc::outcome::result<TokenAmount> getTotalFilCirculationSupply()
        const override;

    /** \copydoc Runtime::getIpfsDatastore() */
    std::shared_ptr<IpfsDatastore> getIpfsDatastore() const override;

    /** \copydoc Runtime::getMessage() */
    std::reference_wrapper<const UnsignedMessage> getMessage() const override;

    outcome::result<void> chargeGas(GasAmount amount) override;

    outcome::result<CID> getCurrentActorState() const override;

    outcome::result<void> commit(const CID &new_state) override;

    outcome::result<boost::optional<Address>> tryResolveAddress(
        const Address &address) const override;

    outcome::result<bool> verifySignature(
        const Signature &signature,
        const Address &address,
        gsl::span<const uint8_t> data) override;

    outcome::result<bool> verifySignatureBytes(
        const Buffer &signature_bytes,
        const Address &address,
        gsl::span<const uint8_t> data) override;

    outcome::result<bool> verifyPoSt(const WindowPoStVerifyInfo &info) override;

    outcome::result<BatchSealsOut> batchVerifySeals(
        const BatchSealsIn &batch) override;

    outcome::result<CID> computeUnsealedSectorCid(
        RegisteredSealProof type,
        const std::vector<PieceInfo> &pieces) override;

    outcome::result<ConsensusFault> verifyConsensusFault(
        const Buffer &block1,
        const Buffer &block2,
        const Buffer &extra) override;

    outcome::result<Blake2b256Hash> hashBlake2b(
        gsl::span<const uint8_t> data) override;

   private:
    std::shared_ptr<Execution> execution_;
    UnsignedMessage message_;
    Address caller_id;
    std::shared_ptr<StateManagerImpl> state_manager;
  };

}  // namespace fc::vm::runtime
