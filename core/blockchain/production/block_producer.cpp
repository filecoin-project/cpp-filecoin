/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/production/block_producer.hpp"

#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "primitives/tipset/load.hpp"
#include "vm/interpreter/interpreter.hpp"

namespace fc::blockchain::production {
  using crypto::signature::BlsSignature;
  using crypto::signature::Secp256k1Signature;
  using primitives::block::MsgMeta;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;

  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  outcome::result<BlockWithMessages> generate(
      InterpreterCache &interpreter_cache,
      const TsLoadPtr &ts_load,
      std::shared_ptr<Ipld> ipld,
      BlockTemplate t) {
    OUTCOME_TRY(parent_tipset, ts_load->load(t.parents));
    OUTCOME_TRY(vm_result, interpreter_cache.get(parent_tipset->key));
    BlockWithMessages b;
    MsgMeta msg_meta;
    cbor_blake::cbLoadT(ipld, msg_meta);
    std::vector<crypto::bls::Signature> bls_signatures;
    for (auto &message : t.messages) {
      OUTCOME_TRY(visit_in_place(
          message.signature,
          [&](const BlsSignature &signature) -> outcome::result<void> {
            b.bls_messages.emplace_back(message.message);
            bls_signatures.push_back(signature);
            OUTCOME_TRY(message_cid, setCbor(ipld, message.message));
            OUTCOME_TRY(msg_meta.bls_messages.append(message_cid));
            return outcome::success();
          },
          [&](const Secp256k1Signature &signature) -> outcome::result<void> {
            b.secp_messages.emplace_back(message);
            OUTCOME_TRY(message_cid, setCbor(ipld, message));
            OUTCOME_TRY(msg_meta.secp_messages.append(message_cid));
            return outcome::success();
          }));
    }
    b.header.miner = std::move(t.miner);
    b.header.ticket = std::move(t.ticket);
    b.header.election_proof = std::move(t.election_proof);
    b.header.beacon_entries = std::move(t.beacon_entries);
    b.header.win_post_proof = std::move(t.win_post_proof);
    static_cast<std::vector<fc::CbCid> &>(b.header.parents) =
        std::move(t.parents);
    b.header.parent_weight = vm_result.weight;
    b.header.height = t.height;
    b.header.parent_state_root = std::move(vm_result.state_root);
    b.header.parent_message_receipts = std::move(vm_result.message_receipts);
    OUTCOME_TRYA(b.header.messages, setCbor(ipld, msg_meta));
    OUTCOME_TRYA(
        b.header.bls_aggregate,
        crypto::bls::BlsProviderImpl{}.aggregateSignatures(bls_signatures));
    b.header.timestamp = t.timestamp;
    // TODO (turuslan): the only caller of "generate" is MinerCreateBlock, it
    // signs block
    b.header.block_sig = {};
    b.header.fork_signaling = 0;
    OUTCOME_TRYA(b.header.parent_base_fee, parent_tipset->nextBaseFee(ipld));
    return std::move(b);
  }
}  // namespace fc::blockchain::production
