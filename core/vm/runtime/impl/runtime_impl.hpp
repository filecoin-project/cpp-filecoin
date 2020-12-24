/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_RUNTIME_IMPL_RUNTIME_IMPL_HPP
#define CPP_FILECOIN_CORE_VM_RUNTIME_IMPL_RUNTIME_IMPL_HPP

#include "vm/actor/invoker.hpp"
#include "vm/runtime/env.hpp"
#include "vm/runtime/runtime.hpp"
#include "vm/state/state_tree.hpp"

namespace fc::vm::runtime {
  using actor::Invoker;
  using state::StateTree;

  class RuntimeImpl : public Runtime {
   public:
    RuntimeImpl(std::shared_ptr<Execution> execution,
                UnsignedMessage message,
                const Address &caller_id);

    std::shared_ptr<Execution> execution() const override;

    NetworkVersion getNetworkVersion() const override;

    /** \copydoc Runtime::getCurrentEpoch() */
    ChainEpoch getCurrentEpoch() const override;

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
    fc::outcome::result<InvocationOutput> send(Address to_address,
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
    std::shared_ptr<IpfsDatastore> getIpfsDatastore() override;

    /** \copydoc Runtime::getMessage() */
    std::reference_wrapper<const UnsignedMessage> getMessage() override;

    outcome::result<void> chargeGas(GasAmount amount) override;

    outcome::result<CID> getCurrentActorState() override;

    outcome::result<void> commit(const CID &new_state) override;

    outcome::result<Address> resolveAddress(const Address &address) override;

    outcome::result<bool> verifySignature(
        const Signature &signature,
        const Address &address,
        gsl::span<const uint8_t> data) override;

    outcome::result<bool> verifySignatureBytes(
        const Buffer &signature_bytes,
        const Address &address,
        gsl::span<const uint8_t> data) override;

    outcome::result<bool> verifyPoSt(const WindowPoStVerifyInfo &info) override;

    outcome::result<std::map<Address, std::vector<bool>>> verifyBatchSeals(
        const adt::Map<adt::Array<SealVerifyInfo>, adt::AddressKeyer> &seals)
        override;

    outcome::result<CID> computeUnsealedSectorCid(
        RegisteredProof type, const std::vector<PieceInfo> &pieces) override;

    outcome::result<ConsensusFault> verifyConsensusFault(
        const Buffer &block1,
        const Buffer &block2,
        const Buffer &extra) override;

   private:
    std::shared_ptr<Execution> execution_;
    UnsignedMessage message_;
    Address caller_id;
  };

}  // namespace fc::vm::runtime

#endif  // CPP_FILECOIN_CORE_VM_RUNTIME_IMPL_RUNTIME_IMPL_HPP
