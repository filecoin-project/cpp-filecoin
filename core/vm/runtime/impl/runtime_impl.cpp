/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/runtime/impl/runtime_impl.hpp"

#include "codec/cbor/cbor.hpp"
#include "primitives/cid/comm_cid.hpp"
#include "proofs/proofs.hpp"
#include "vm/actor/builtin/account/account_actor.hpp"
#include "vm/runtime/gas_cost.hpp"
#include "vm/runtime/impl/actor_state_handle_impl.hpp"
#include "vm/runtime/runtime_error.hpp"

namespace fc::vm::runtime {

  using fc::crypto::randomness::ChainEpoch;
  using fc::crypto::randomness::Randomness;
  using fc::crypto::randomness::Serialization;
  using fc::primitives::BigInt;
  using fc::primitives::address::Address;
  using fc::primitives::address::Protocol;
  using fc::storage::hamt::HamtError;
  using fc::storage::ipfs::IpfsDatastore;
  using fc::vm::actor::Actor;
  using fc::vm::actor::ActorSubstateCID;
  using fc::vm::actor::CodeId;
  using fc::vm::actor::MethodNumber;
  using fc::vm::message::UnsignedMessage;

  RuntimeImpl::RuntimeImpl(std::shared_ptr<Execution> execution,
                           UnsignedMessage message,
                           const Address &caller_id,
                           ActorSubstateCID current_actor_state)
      : execution_{std::move(execution)},
        state_tree_{execution_->state_tree},
        message_{std::move(message)},
        caller_id{caller_id},
        current_actor_state_{std::move(current_actor_state)} {}

  ChainEpoch RuntimeImpl::getCurrentEpoch() const {
    return execution_->env->chain_epoch;
  }

  Randomness RuntimeImpl::getRandomness(DomainSeparationTag tag,
                                        ChainEpoch epoch) const {
    return execution_->env->randomness_provider->deriveRandomness(
        tag, Serialization{}, epoch);
  }

  Randomness RuntimeImpl::getRandomness(DomainSeparationTag tag,
                                        ChainEpoch epoch,
                                        Serialization seed) const {
    return execution_->env->randomness_provider->deriveRandomness(
        tag, seed, epoch);
  }

  Address RuntimeImpl::getImmediateCaller() const {
    return caller_id;
  }

  Address RuntimeImpl::getCurrentReceiver() const {
    return message_.to;
  }

  std::shared_ptr<ActorStateHandle> RuntimeImpl::acquireState() const {
    return std::make_shared<ActorStateHandleImpl>();
  }

  fc::outcome::result<BigInt> RuntimeImpl::getBalance(
      const Address &address) const {
    auto actor_state = state_tree_->get(address);
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
    OUTCOME_TRY(actor_state, state_tree_->get(address));
    return actor_state.code;
  }

  fc::outcome::result<InvocationOutput> RuntimeImpl::send(
      Address to_address,
      MethodNumber method_number,
      MethodParams params,
      BigInt value) {
    return execution_->sendWithRevert(
        {0, to_address, message_.to, {}, value, {}, {}, method_number, params});
  }

  fc::outcome::result<void> RuntimeImpl::createActor(const Address &address,
                                                     const Actor &actor) {
    OUTCOME_TRY(state_tree_->set(address, actor));
    return fc::outcome::success();
  }

  fc::outcome::result<void> RuntimeImpl::deleteActor() {
    OUTCOME_TRY(chargeGas(kDeleteActorGasCost));
    // TODO: transfer to kBurntFundsActorAddress
    // TODO(a.chernyshov) FIL-137 implement state_tree remove if needed
    // return state_tree_->remove(address);
    return fc::outcome::failure(RuntimeError::kUnknown);
  }

  std::shared_ptr<IpfsDatastore> RuntimeImpl::getIpfsDatastore() {
    // TODO(turuslan): FIL-131 charging store
    return state_tree_->getStore();
  }

  std::reference_wrapper<const UnsignedMessage> RuntimeImpl::getMessage() {
    return message_;
  }

  ActorSubstateCID RuntimeImpl::getCurrentActorState() {
    return current_actor_state_;
  }

  fc::outcome::result<void> RuntimeImpl::commit(
      const ActorSubstateCID &new_state) {
    OUTCOME_TRY(chargeGas(kCommitGasCost));
    current_actor_state_ = new_state;
    return outcome::success();
  }

  fc::outcome::result<void> RuntimeImpl::transfer(Actor &from,
                                                  Actor &to,
                                                  const BigInt &amount) {
    if (from.balance < amount) return RuntimeError::kNotEnoughFunds;
    from.balance = from.balance - amount;
    to.balance = to.balance + amount;
    return outcome::success();
  }

  fc::outcome::result<Address> RuntimeImpl::resolveAddress(
      const Address &address) {
    return state_tree_->lookupId(address);
  }

  outcome::result<bool> RuntimeImpl::verifySignature(
      const Signature &signature,
      const Address &address,
      gsl::span<const uint8_t> data) {
    // TODO(turuslan): implement
    return RuntimeError::kUnknown;
  }

  outcome::result<bool> RuntimeImpl::verifyPoSt(
      const WindowPoStVerifyInfo &info) {
    WindowPoStVerifyInfo preprocess_info = info;
    preprocess_info.randomness[31] = 0;
    return proofs::Proofs::verifyWindowPoSt(preprocess_info);
  }

  fc::outcome::result<bool> RuntimeImpl::verifySeal(
      const SealVerifyInfo &info) {
    return proofs::Proofs::verifySeal(info);
  }

  fc::outcome::result<fc::CID> RuntimeImpl::computeUnsealedSectorCid(
      RegisteredProof type, const std::vector<PieceInfo> &pieces) {
    constexpr auto levels = 37u;
    constexpr auto skip = 2u;
    static auto c = [](auto s) { return common::Comm::fromHex(s).value(); };
    static const common::Comm comms[levels - skip]{
        c("3731bb99ac689f66eef5973e4a94da188f4ddcae580724fc6f3fd60dfd488333"),
        c("642a607ef886b004bf2c1978463ae1d4693ac0f410eb2d1b7a47fe205e5e750f"),
        c("57a2381a28652bf47f6bef7aca679be4aede5871ab5cf3eb2c08114488cb8526"),
        c("1f7ac9595510e09ea41c460b176430bb322cd6fb412ec57cb17d989a4310372f"),
        c("fc7e928296e516faade986b28f92d44a4f24b935485223376a799027bc18f833"),
        c("08c47b38ee13bc43f41b915c0eed9911a26086b3ed62401bf9d58b8d19dff624"),
        c("b2e47bfb11facd941f62af5c750f3ea5cc4df517d5c4f16db2b4d77baec1a32f"),
        c("f9226160c8f927bfdcc418cdf203493146008eaefb7d02194d5e548189005108"),
        c("2c1a964bb90b59ebfe0f6da29ad65ae3e417724a8f7c11745a40cac1e5e74011"),
        c("fee378cef16404b199ede0b13e11b624ff9d784fbbed878d83297e795e024f02"),
        c("8e9e2403fa884cf6237f60df25f83ee40dca9ed879eb6f6352d15084f5ad0d3f"),
        c("752d9693fa167524395476e317a98580f00947afb7a30540d625a9291cc12a07"),
        c("7022f60f7ef6adfa17117a52619e30cea82c68075adf1c667786ec506eef2d19"),
        c("d99887b973573a96e11393645236c17b1f4c7034d723c7a99f709bb4da61162b"),
        c("d0b530dbb0b4f25c5d2f2a28dfee808b53412a02931f18c499f5a254086b1326"),
        c("84c0421ba0685a01bf795a2344064fe424bd52a9d24377b394ff4c4b4568e811"),
        c("65f29e5d98d246c38b388cfc06db1f6b021303c5a289000bdce832a9c3ec421c"),
        c("a2247508285850965b7e334b3127b0c042b1d046dc54402137627cd8799ce13a"),
        c("dafdab6da9364453c26d33726b9fefe343be8f81649ec009aad3faff50617508"),
        c("d941d5e0d6314a995c33ffbd4fbe69118d73d4e5fd2cd31f0f7c86ebdd14e706"),
        c("514c435c3d04d349a5365fbd59ffc713629111785991c1a3c53af22079741a2f"),
        c("ad06853969d37d34ff08e09f56930a4ad19a89def60cbfee7e1d3381c1e71c37"),
        c("39560e7b13a93b07a243fd2720ffa7cb3e1d2e505ab3629e79f46313512cda06"),
        c("ccc3c012f5b05e811a2bbfdd0f6833b84275b47bf229c0052a82484f3c1a5b3d"),
        c("7df29b69773199e8f2b40b77919d048509eed768e2c7297b1f1437034fc3c62c"),
        c("66ce05a3667552cf45c02bcc4e8392919bdeac35de2ff56271848e9f7b675107"),
        c("d8610218425ab5e95b1ca6239d29a2e420d706a96f373e2f9c9a91d759d19b01"),
        c("6d364b1ef846441a5a4a68862314acc0a46f016717e53443e839eedf83c2853c"),
        c("077e5fde35c50a9303a55009e3498a4ebedff39c42b710b730d8ec7ac7afa63e"),
        c("e64005a6bfe3777953b8ad6ef93f0fca1049b2041654f2a411f7702799cece02"),
        c("259d3d6b1f4d876d1185e1123af6f5501af0f67cf15b5216255b7b178d12051d"),
        c("3f9a4d411da4ef1b36f35ff0a195ae392ab23fee7967b7c41b03d1613fc29239"),
        c("fe4ef328c61aa39cfdb2484eaa32a151b1fe3dfd1f96dd8c9711fd86d6c58113"),
        c("f55d68900e2d8381eccb8164cb9976f24b2de0dd61a31b97ce6eb23850d5e819"),
        c("aaaa8c4cb40aacee1e02dc65424b2a6c8e99f803b72f7929c4101d7fae6bff32"),
    };
    OUTCOME_TRY(left, primitives::sector::getSectorSize(type));
    auto pieces2 = pieces;
    for (auto piece : pieces) {
      left -= piece.size;
    }
    for (uint64_t m = 1; left; m <<= 1) {
      if (left & m) {
        left ^= m;
        auto p = primitives::piece::PaddedPieceSize{m}.unpadded().padded();
        auto z = p ? 0u : 64u;
        for (uint64_t n = p; !(n & 1); n >>= 1) {
          ++z;
        }
        auto level = z - skip - 5;
        assert(level >= 0 && level < std::size(comms));
        pieces2.push_back({p, common::pieceCommitmentV1ToCID(comms[level])});
      }
    }
    return proofs::Proofs::generateUnsealedCID(type, pieces2);
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
