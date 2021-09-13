/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/block_validator/impl/block_validator_impl.hpp"

#include "blockchain/block_validator/impl/consensus_rules.hpp"
#include "blockchain/block_validator/impl/syntax_rules.hpp"
#include "codec/cbor/cbor_codec.hpp"
#include "primitives/cid/cid_of_cbor.hpp"
#include "storage/amt/amt.hpp"

namespace fc::blockchain::block_validator {
  using primitives::address::Protocol;
  using storage::amt::Amt;
  using SignedMessage = vm::message::SignedMessage;
  using UnsignedMessage = vm::message::UnsignedMessage;
  using BlsCryptoSignature = crypto::bls::Signature;
  using BlsCryptoPubKey = crypto::bls::PublicKey;
  using SecpCryptoSignature = crypto::secp256k1::Signature;
  using SecpCryptoPubKey = crypto::secp256k1::PublicKey;

  const std::map<scenarios::Stage, BlockValidatorImpl::StageExecutor>
      BlockValidatorImpl::stage_executors_{
          {Stage::SYNTAX_BV0, &BlockValidatorImpl::syntax},
          {Stage::CONSENSUS_BV1, &BlockValidatorImpl::consensus},
          {Stage::BLOCK_SIGNATURE_BV2, &BlockValidatorImpl::blockSign},
          {Stage::ELECTION_POST_BV3, &BlockValidatorImpl::electionPost},
          {Stage::MESSAGE_SIGNATURE_BV4, &BlockValidatorImpl::messageSign},
          {Stage::STATE_TREE_BV5, &BlockValidatorImpl::stateTree}};

  outcome::result<void> BlockValidatorImpl::validateBlock(
      const BlockHeader &block, scenarios::Scenario scenario) const {
    for (const auto &stage : scenario) {
      auto executor = stage_executors_.find(stage);
      if (executor != stage_executors_.end()) {
        auto result = std::invoke(executor->second, this, block);
        if (result.has_error()) {
          return result;
        }
      } else {
        return ValidatorError::kUnknownStage;
      }
    }
    return outcome::success();
  }

  outcome::result<void> BlockValidatorImpl::syntax(
      const BlockHeader &block) const {
    OUTCOME_TRY(SyntaxRules::parentsCount(block));
    OUTCOME_TRY(SyntaxRules::parentWeight(block));
    OUTCOME_TRY(SyntaxRules::minerAddress(block));
    OUTCOME_TRY(SyntaxRules::timestamp(block));
    OUTCOME_TRY(SyntaxRules::ticket(block));
    OUTCOME_TRY(SyntaxRules::electionPost(block));
    OUTCOME_TRY(SyntaxRules::forkSignal(block));
    return outcome::success();
  }

  outcome::result<void> BlockValidatorImpl::consensus(
      const BlockHeader &block) const {
    OUTCOME_TRY(ConsensusRules::activeMiner(block, power_table_));
    OUTCOME_TRY(parent_tipset, getParentTipset(block));
    OUTCOME_TRY(ConsensusRules::parentWeight(
        block, *parent_tipset.get(), weight_calculator_));
    OUTCOME_TRY(chain_epoch, epoch_clock_->epochAtTime(clock_->nowUTC()));
    OUTCOME_TRY(ConsensusRules::epoch(block, chain_epoch));
    return outcome::success();
  }

  outcome::result<void> BlockValidatorImpl::blockSign(
      const BlockHeader &block) const {
    using SecpPubKey = primitives::address::Secp256k1PublicKeyHash;
    using BlsPubKey = primitives::address::BLSPublicKeyHash;
    using ActorExecHash = primitives::address::ActorExecHash;
    const auto &block_signature = block.block_sig.value();
    OUTCOME_TRY(block_bytes, codec::cbor::encode(block));
    auto validation_result = visit_in_place(
        block.miner.data,
        [](uint64_t) -> outcome::result<void> {
          return ValidatorError::kInvalidMinerPublicKey;
        },
        [](const ActorExecHash &) -> outcome::result<void> {
          return ValidatorError::kInvalidMinerPublicKey;
        },
        [&block_signature, &block_bytes, *this](
            const SecpPubKey &public_key) -> outcome::result<void> {
          crypto::secp256k1::PublicKey secp_public_key;
          auto secp_signature =
              boost::get<crypto::secp256k1::Signature>(block_signature);
          std::copy_n(public_key.begin(),
                      secp_public_key.size(),
                      secp_public_key.begin());
          OUTCOME_TRY(result,
                      this->secp_provider_->verify(gsl::make_span(block_bytes),
                                                   secp_signature,
                                                   secp_public_key));
          if (result) {
            return outcome::success();
          }
          return ValidatorError::kInvalidBlockSignature;
        },
        [&block_signature, &block_bytes, *this](
            const BlsPubKey &public_key) -> outcome::result<void> {
          auto bls_signature =
              boost::get<crypto::bls::Signature>(block_signature);
          crypto::bls::PublicKey bls_public_key;
          std::copy_n(public_key.begin(),
                      bls_public_key.size(),
                      bls_public_key.begin());
          OUTCOME_TRY(
              result,
              this->bls_provider_->verifySignature(
                  gsl::make_span(block_bytes), bls_signature, bls_public_key));
          if (result) {
            return outcome::success();
          }
          return ValidatorError::kInvalidBlockSignature;
        });
    return validation_result;
  }

  outcome::result<void> BlockValidatorImpl::electionPost(
      const BlockHeader &block) const {
    return outcome::success();
  }

  outcome::result<void> BlockValidatorImpl::chainAncestry(
      const BlockHeader &block) const {
    return outcome::success();
  }

  outcome::result<void> BlockValidatorImpl::messageSign(
      const BlockHeader &block) const {
    return outcome::success();
  }

  outcome::result<void> BlockValidatorImpl::stateTree(
      const BlockHeader &block) const {
    OUTCOME_TRY(parent_tipset, getParentTipset(block));
    OUTCOME_TRY(result,
                interpreter_cache_->get(
                    InterpreterCache::Key{parent_tipset.get()->key}));
    if (result.state_root == block.parent_state_root
        && result.message_receipts == block.parent_message_receipts) {
      return outcome::success();
    }
    return ValidatorError::kInvalidParentState;
  }

  outcome::result<std::reference_wrapper<BlockValidatorImpl::TipsetCPtr>>
  BlockValidatorImpl::getParentTipset(const BlockHeader &block) const {
    auto cid{primitives::cid::getCidOfCbor(block).value()};
    if (parent_tipset_cache_ && parent_tipset_cache_.value().first == cid) {
      return parent_tipset_cache_.value().second;
    }
    std::vector<BlockHeader> parent_blocks;
    for (const auto &parent_block_cid : block.parents) {
      auto block_bytes_response = datastore_->get(CID{parent_block_cid});
      if (block_bytes_response.has_value()) {
        auto block_header_result =
            codec::cbor::decode<BlockHeader>(block_bytes_response.value());
        if (block_header_result.has_value()) {
          parent_blocks.emplace_back(std::move(block_header_result.value()));
        } else {
          return ConsensusError::kGetParentTipsetError;
        }
      } else {
        return ConsensusError::kGetParentTipsetError;
      }
    }
    OUTCOME_TRY(tipset, Tipset::create(parent_blocks));
    parent_tipset_cache_ = std::make_pair(std::move(cid), std::move(tipset));
    return parent_tipset_cache_.value().second;
  }

}  // namespace fc::blockchain::block_validator

OUTCOME_CPP_DEFINE_CATEGORY(fc::blockchain::block_validator,
                            ValidatorError,
                            e) {
  using fc::blockchain::block_validator::ValidatorError;
  switch (e) {
    case ValidatorError::kUnknownStage:
      return "Block validation: unknown validation stage";
    case ValidatorError::kUnknownBlockSignature:
      return "Block validation: unknown block signature";
    case ValidatorError::kInvalidBlockSignature:
      return "Block validation: invalid block signature";
    case ValidatorError::kInvalidMinerPublicKey:
      return "Block validation: invalid miner public key";
    case ValidatorError::kInvalidParentState:
      return "Block validation: invalid parent state";
  }
  return "Block validation: unknown error";
}
