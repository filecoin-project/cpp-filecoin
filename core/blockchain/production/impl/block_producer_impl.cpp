/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/production/impl/block_producer_impl.hpp"

#include <vector>

#include <gsl/span>
#include "clock/chain_epoch_clock.hpp"
#include "codec/cbor/cbor.hpp"
#include "common/visitor.hpp"
#include "primitives/cid/cid_of_cbor.hpp"
#include "storage/amt/amt.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"

namespace fc::blockchain::production {
  using clock::Time;
  using codec::cbor::decode;
  using crypto::signature::BlsSignature;
  using crypto::signature::Secp256k1Signature;
  using primitives::block::BlockHeader;
  using storage::amt::Amt;
  using storage::amt::Root;
  using storage::ipfs::InMemoryDatastore;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;

  BlockProducerImpl::BlockProducerImpl(
      std::shared_ptr<IpfsDatastore> data_store,
      std::shared_ptr<MessageStorage> message_store,
      std::shared_ptr<UTCClock> utc_clock,
      std::shared_ptr<ChainEpochClock> epoch_clock,
      std::shared_ptr<WeightCalculator> weight_calculator,
      std::shared_ptr<BlsProvider> crypto_provider,
      std::shared_ptr<Interpreter> interpreter)
      : data_storage_{std::move(data_store)},
        message_storage_{std::move(message_store)},
        clock_{std::move(utc_clock)},
        epoch_{std::move(epoch_clock)},
        chain_weight_calculator_{std::move(weight_calculator)},
        bls_provider_{std::move(crypto_provider)},
        vm_interpreter_{std::move(interpreter)} {}

  outcome::result<BlockProducer::Block> BlockProducerImpl::generate(
      primitives::address::Address miner_address,
      const CID &parent_tipset_id,
      EPostProof proof,
      Ticket ticket,
      std::shared_ptr<Indices> indices) {
    OUTCOME_TRY(parent_tipset, getTipset(parent_tipset_id));
    OUTCOME_TRY(
        vm_result,
        vm_interpreter_->interpret(data_storage_, parent_tipset, indices));
    OUTCOME_TRY(parent_weight,
                chain_weight_calculator_->calculateWeight(parent_tipset));
    std::vector<SignedMessage> messages =
        message_storage_->getTopScored(config::kBlockMaxMessagesCount);
    OUTCOME_TRY(msg_meta, getMessagesMeta(messages));
    OUTCOME_TRY(data_storage_->set(msg_meta.getCID(),
    msg_meta.getRawBytes()));
    std::vector<UnsignedMessage> bls_messages;
    std::vector<SignedMessage> secp_messages;
    std::vector<crypto::bls::Signature> bls_signatures;
    for (auto &&message : messages) {
      visit_in_place(
          message.signature,
          [&message, &bls_messages, &bls_signatures](
              const BlsSignature &signature) {
            bls_messages.emplace_back(std::move(message.message));
            bls_signatures.push_back(signature);
          },
          [&message, &secp_messages](const Secp256k1Signature &signature) {
            secp_messages.emplace_back(std::move(message));
          });
    }
    OUTCOME_TRY(bls_aggregate_sign,
                bls_provider_->aggregateSignatures(bls_signatures));
    Time now = clock_->nowUTC();
    OUTCOME_TRY(current_epoch, epoch_->epochAtTime(now));
    BlockHeader header;
    header.miner = std::move(miner_address);
    header.ticket = ticket;
    header.epost_proof = std::move(proof);
    header.parents = std::move(parent_tipset.cids);
    header.parent_weight = parent_weight;
    header.height = static_cast<uint64_t>(current_epoch);
    header.parent_state_root = std::move(vm_result.state_root);
    header.parent_message_receipts = std::move(vm_result.message_receipts);
    header.messages = msg_meta.getCID();
    header.bls_aggregate = std::move(bls_aggregate_sign);
    header.timestamp = static_cast<uint64_t>(now.unixTime().count());
    header.block_sig = {};  // Block must be signed be Actor Miner
    header.fork_signaling = 0;
    return Block{.header = std::move(header),
                 .bls_messages = std::move(bls_messages),
                 .secp_messages = std::move(secp_messages)};
  }

  outcome::result<BlockProducerImpl::Tipset> BlockProducerImpl::getTipset(
      const CID &tipset_id) const {
    auto raw_data = data_storage_->get(tipset_id);
    if (raw_data.has_error()) {
      return BlockProducerError::PARENT_TIPSET_NOT_FOUND;
    }
    auto tipset = codec::cbor::decode<Tipset>(raw_data.value());
    if (tipset.has_error()) {
      return BlockProducerError::PARENT_TIPSET_INVALID_CONTENT;
    }
    return tipset.value();
  }

  outcome::result<BlockProducerImpl::MsgMeta>
  BlockProducerImpl::getMessagesMeta(
      const std::vector<SignedMessage> &messages) {
    auto bls_backend = std::make_shared<InMemoryDatastore>();
    auto secp_backend = std::make_shared<InMemoryDatastore>();
    Amt bls_messages_amt{bls_backend};
    Amt secp_messages_amt{secp_backend};
    std::vector<SignedMessage> bls_messages;
    std::vector<SignedMessage> secp_messages;
    for (const auto &msg : messages) {
      visit_in_place(
          msg.signature,
          [&msg, &bls_messages](const BlsSignature &signature) {
            std::ignore = signature;
            bls_messages.push_back(msg);
          },
          [&msg, &secp_messages](const Secp256k1Signature &signature) {
            std::ignore = signature;
            secp_messages.push_back(msg);
          });
    }
    for (size_t index = 0; index < bls_messages.size(); ++index) {
      OUTCOME_TRY(message_cid,
                  primitives::cid::getCidOfCbor(bls_messages.at(index)));
      OUTCOME_TRY(bls_messages_amt.setCbor(index, message_cid));
    }
    for (size_t index = 0; index < secp_messages.size(); ++index) {
      OUTCOME_TRY(message_cid,
                  primitives::cid::getCidOfCbor(secp_messages.at(index)));
      OUTCOME_TRY(secp_messages_amt.setCbor(index, message_cid));
    }
    OUTCOME_TRY(bls_root_cid, bls_messages_amt.flush());
    OUTCOME_TRY(secp_root_cid, secp_messages_amt.flush());
    MsgMeta msg_meta{};
    msg_meta.bls_messages = bls_root_cid;
    msg_meta.secpk_messages = secp_root_cid;
    return msg_meta;
  }
}  // namespace fc::blockchain::production

OUTCOME_CPP_DEFINE_CATEGORY(fc::blockchain::production, BlockProducerError, e) {
  using fc::blockchain::production::BlockProducerError;
  switch (e) {
    case (BlockProducerError::PARENT_TIPSET_NOT_FOUND):
      return "Block Generator: failed to load parent tipset";
    case (BlockProducerError::PARENT_TIPSET_INVALID_CONTENT):
      return "Block Generator: failed to decode parent tipset content";
  }
  return "Block Generator: unknown error";
}
