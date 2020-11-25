/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/runtime/impl/runtime_impl.hpp"

#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "crypto/secp256k1/impl/secp256k1_provider_impl.hpp"
#include "primitives/cid/comm_cid.hpp"
#include "proofs/proofs.hpp"
#include "storage/keystore/impl/in_memory/in_memory_keystore.hpp"
#include "vm/actor/builtin/account/account_actor.hpp"
#include "vm/runtime/env.hpp"
#include "vm/runtime/runtime_error.hpp"
#include "vm/version.hpp"

namespace fc::vm::runtime {
  using fc::primitives::BigInt;
  using fc::primitives::address::Protocol;
  using fc::storage::hamt::HamtError;

  RuntimeImpl::RuntimeImpl(std::shared_ptr<Execution> execution,
                           UnsignedMessage message,
                           const Address &caller_id)
      : execution_{std::move(execution)},
        message_{std::move(message)},
        caller_id{caller_id} {}

  std::shared_ptr<Execution> RuntimeImpl::execution() const {
    return execution_;
  }

  NetworkVersion RuntimeImpl::getNetworkVersion() const {
    return version::getNetworkVersion(getCurrentEpoch());
  }

  ChainEpoch RuntimeImpl::getCurrentEpoch() const {
    return execution_->env->tipset.height;
  }

  outcome::result<Randomness> RuntimeImpl::getRandomnessFromTickets(
      DomainSeparationTag tag,
      ChainEpoch epoch,
      gsl::span<const uint8_t> seed) const {
    return execution_->env->tipset.ticketRandomness(
        *execution_->env->ipld, tag, epoch, seed);
  }

  outcome::result<Randomness> RuntimeImpl::getRandomnessFromBeacon(
      DomainSeparationTag tag,
      ChainEpoch epoch,
      gsl::span<const uint8_t> seed) const {
    // TODO (a.chernyshov) implement
    // test vector randomness
    return crypto::randomness::Randomness::fromString(
        "i_am_random_____i_am_random_____");
  }

  Address RuntimeImpl::getImmediateCaller() const {
    return caller_id;
  }

  Address RuntimeImpl::getCurrentReceiver() const {
    return message_.to;
  }

  fc::outcome::result<BigInt> RuntimeImpl::getBalance(
      const Address &address) const {
    auto actor_state = execution_->state_tree->get(address);
    if (!actor_state) {
      if (actor_state.error() == HamtError::kNotFound) return BigInt(0);
      return actor_state.error();
    }
    return actor_state.value().balance;
  }

  BigInt RuntimeImpl::getValueReceived() const {
    return message_.value;
  }

  fc::outcome::result<CodeId> RuntimeImpl::getActorCodeID(
      const Address &address) const {
    OUTCOME_TRY(actor_state, execution_->state_tree->get(address));
    return actor_state.code;
  }

  fc::outcome::result<InvocationOutput> RuntimeImpl::send(
      Address to_address,
      MethodNumber method_number,
      MethodParams params,
      BigInt value) {
    return execution_->sendWithRevert(
        {to_address, message_.to, {}, value, {}, {}, method_number, params});
  }

  outcome::result<Address> RuntimeImpl::createNewActorAddress() {
    OUTCOME_TRY(caller_address,
                resolveKey(*execution_->state_tree, execution()->origin));
    OUTCOME_TRY(encoded_address, codec::cbor::encode(caller_address));
    auto actor_address{Address::makeActorExec(
        encoded_address.putUint64(execution()->origin_nonce)
            .putUint64(execution_->actors_created))};

    ++execution_->actors_created;
    return actor_address;
  }

  fc::outcome::result<void> RuntimeImpl::createActor(const Address &address,
                                                     const Actor &actor) {
    OUTCOME_TRY(execution_->state_tree->set(address, actor));
    OUTCOME_TRY(chargeGas(execution_->env->pricelist.onCreateActor()));
    return fc::outcome::success();
  }

  fc::outcome::result<void> RuntimeImpl::deleteActor(const Address &address) {
    OUTCOME_TRY(chargeGas(execution_->env->pricelist.onDeleteActor()));
    auto &state{*execution()->state_tree};
    if (auto _actor{state.get(getCurrentReceiver())}) {
      if (_actor.has_error() && _actor.error() == HamtError::kNotFound) {
        return VMExitCode::kSysErrorIllegalActor;
      }
      const auto &balance{_actor.value().balance};
      if (balance.is_zero()
          || transfer(getCurrentReceiver(), address, balance)) {
        if (state.remove(getCurrentReceiver())) {
          return outcome::success();
        }
      }
    }
    return VMExitCode::kSysErrorIllegalActor;
  }

  fc::outcome::result<void> RuntimeImpl::transfer(const Address &debitFrom,
                                                  const Address &creditTo,
                                                  const TokenAmount &amount) {
    if (amount < 0) {
      return VMExitCode::kSysErrForbidden;
    }

    auto &state{*execution()->state_tree};

    OUTCOME_TRY(from_id, state.lookupId(debitFrom));
    OUTCOME_TRY(to_id, state.lookupId(creditTo));
    if (from_id != to_id) {
      OUTCOME_TRY(from_actor, state.get(from_id));
      OUTCOME_TRY(to_actor, state.get(to_id));

      if (from_actor.balance < amount) {
        return VMExitCode::kSysErrInsufficientFunds;
      }

      from_actor.balance -= amount;
      to_actor.balance += amount;
      OUTCOME_TRY(state.set(from_id, from_actor));
      OUTCOME_TRY(state.set(to_id, to_actor));
    }

    return outcome::success();
  }

  fc::outcome::result<TokenAmount> RuntimeImpl::getTotalFilCirculationSupply()
      const {
    // TODO(a.chernyshov) implement
    // 0 is for test vectors
    return 0;
  }

  std::shared_ptr<IpfsDatastore> RuntimeImpl::getIpfsDatastore() {
    return execution_->charging_ipld;
  }

  std::reference_wrapper<const UnsignedMessage> RuntimeImpl::getMessage() {
    return message_;
  }

  outcome::result<CID> RuntimeImpl::getCurrentActorState() {
    OUTCOME_TRY(actor, execution_->state_tree->get(getCurrentReceiver()));
    return actor.head;
  }

  fc::outcome::result<void> RuntimeImpl::commit(const CID &new_state) {
    OUTCOME_TRY(actor, execution_->state_tree->get(getCurrentReceiver()));
    actor.head = new_state;
    OUTCOME_TRY(execution_->state_tree->set(getCurrentReceiver(), actor));
    return outcome::success();
  }

  fc::outcome::result<Address> RuntimeImpl::resolveAddress(
      const Address &address) {
    return execution_->state_tree->lookupId(address);
  }

  outcome::result<bool> RuntimeImpl::verifySignature(
      const Signature &signature,
      const Address &address,
      gsl::span<const uint8_t> data) {
    OUTCOME_TRY(
        chargeGas(execution_->env->pricelist.onVerifySignature(signature.isBls())));
    OUTCOME_TRY(account,
                resolveKey(*execution_->state_tree, address));
    return storage::keystore::InMemoryKeyStore{
        std::make_shared<crypto::bls::BlsProviderImpl>(),
        std::make_shared<crypto::secp256k1::Secp256k1ProviderImpl>()}
        .verify(account, data, signature);
  }

  outcome::result<bool> RuntimeImpl::verifyPoSt(
      const WindowPoStVerifyInfo &info) {
    WindowPoStVerifyInfo preprocess_info = info;
    preprocess_info.randomness[31] = 0;
    return proofs::Proofs::verifyWindowPoSt(preprocess_info);
  }

  fc::outcome::result<fc::CID> RuntimeImpl::computeUnsealedSectorCid(
      RegisteredProof type, const std::vector<PieceInfo> &pieces) {
    OUTCOME_TRY(
        chargeGas(execution_->env->pricelist.onComputeUnsealedSectorCid()));
    return proofs::Proofs::generateUnsealedCID(type, pieces, true);
  }

  fc::outcome::result<ConsensusFault> RuntimeImpl::verifyConsensusFault(
      const Buffer &block1, const Buffer &block2, const Buffer &extra) {
    // TODO(a.chernyshov): implement
    return RuntimeError::kUnknown;
  }

  fc::outcome::result<void> RuntimeImpl::chargeGas(GasAmount amount) {
    return execution_->chargeGas(amount);
  }
}  // namespace fc::vm::runtime
