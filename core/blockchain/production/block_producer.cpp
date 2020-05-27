/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/production/block_producer.hpp"

#include "blockchain/impl/weight_calculator_impl.hpp"
#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "vm/interpreter/impl/interpreter_impl.hpp"

namespace fc::blockchain::production {
  using crypto::signature::BlsSignature;
  using crypto::signature::Secp256k1Signature;
  using primitives::block::MsgMeta;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;

  outcome::result<Block> generate(std::shared_ptr<Ipld> ipld, BlockTemplate t) {
    OUTCOME_TRY(parent_tipset,
                primitives::tipset::Tipset::load(*ipld, t.parents));
    OUTCOME_TRY(
        vm_result,
        vm::interpreter::InterpreterImpl{}.interpret(ipld, parent_tipset));
    Block b;
    MsgMeta msg_meta;
    msg_meta.load(ipld);
    std::vector<crypto::bls::Signature> bls_signatures;
    for (auto &message : t.messages) {
      OUTCOME_TRY(visit_in_place(
          message.signature,
          [&](const BlsSignature &signature) -> outcome::result<void> {
            b.bls_messages.emplace_back(message.message);
            bls_signatures.push_back(signature);
            OUTCOME_TRY(message_cid, ipld->setCbor(message.message));
            OUTCOME_TRY(msg_meta.bls_messages.append(message_cid));
            return outcome::success();
          },
          [&](const Secp256k1Signature &signature) -> outcome::result<void> {
            b.secp_messages.emplace_back(message);
            OUTCOME_TRY(message_cid, ipld->setCbor(message));
            OUTCOME_TRY(msg_meta.secp_messages.append(message_cid));
            return outcome::success();
          }));
    }
    b.header.miner = std::move(t.miner);
    b.header.ticket = std::move(t.ticket);
    b.header.election_proof = std::move(t.election_proof);
    b.header.beacon_entries = std::move(t.beacon_entries);
    b.header.win_post_proof = std::move(t.win_post_proof);
    b.header.parents = std::move(t.parents);
    OUTCOME_TRYA(
        b.header.parent_weight,
        weight::WeightCalculatorImpl{ipld}.calculateWeight(parent_tipset));
    b.header.height = t.height;
    b.header.parent_state_root = std::move(vm_result.state_root);
    b.header.parent_message_receipts = std::move(vm_result.message_receipts);
    OUTCOME_TRY(msg_meta.flush());
    OUTCOME_TRYA(b.header.messages, ipld->setCbor(msg_meta));
    OUTCOME_TRYA(
        b.header.bls_aggregate,
        crypto::bls::BlsProviderImpl{}.aggregateSignatures(bls_signatures));
    b.header.timestamp = t.timestamp;
    // TODO: the only caller of "generate" is MinerCreateBlock, it signs block
    b.header.block_sig = {};
    b.header.fork_signaling = 0;
    return std::move(b);
  }
}  // namespace fc::blockchain::production
