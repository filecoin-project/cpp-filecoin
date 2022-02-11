/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/block_validator/validator.hpp"

#include "blockchain/block_validator/eligible.hpp"
#include "blockchain/block_validator/win_sectors.hpp"
#include "cbor_blake/ipld_any.hpp"
#include "cbor_blake/ipld_version.hpp"
#include "cbor_blake/memory.hpp"
#include "common/error_text.hpp"
#include "common/from_span.hpp"
#include "common/prometheus/metrics.hpp"
#include "common/prometheus/since.hpp"
#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "primitives/block/rand.hpp"
#include "primitives/tipset/chain.hpp"
#include "storage/map_prefix/prefix.hpp"
#include "vm/actor/builtin/states/miner/miner_actor_state.hpp"
#include "vm/actor/builtin/states/storage_power/storage_power_actor_state.hpp"
#include "vm/interpreter/interpreter.hpp"
#include "vm/message/impl/message_signer_impl.hpp"
#include "vm/message/valid.hpp"
#include "vm/runtime/pricelist.hpp"
#include "vm/state/impl/state_tree_impl.hpp"
#include "vm/state/resolve_key.hpp"
#include "vm/toolchain/toolchain.hpp"

namespace fc::blockchain::block_validator {
  using primitives::Nonce;
  using primitives::block::MsgMeta;
  using primitives::sector::SectorInfo;
  using primitives::sector::toSectorInfo;
  using vm::actor::kStoragePowerAddress;
  using vm::actor::builtin::states::MinerActorStatePtr;
  using vm::actor::builtin::states::PowerActorStatePtr;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;
  using vm::version::getNetworkVersion;
  using vm::version::NetworkVersion;

  BlockValidator::BlockValidator(MapPtr kv, const EnvironmentContext &envx)
      : kv{std::move(kv)},
        ipld{envx.ipld},
        ts_load{envx.ts_load},
        interpreter_cache{envx.interpreter_cache},
        ts_branches_mutex{envx.ts_branches_mutex} {}

  outcome::result<void> BlockValidator::validate(const TsBranchPtr &branch,
                                                 const BlockHeader &block) {
    static auto &metricTime{prometheus::BuildHistogram()
                                .Name("lotus_block_validation_ms")
                                .Help("Duration for Block Validation in ms")
                                .Register(prometheusRegistry())
                                .Add({}, kDefaultPrometheusMsBuckets)};
    auto BOOST_OUTCOME_TRY_UNIQUE_NAME{
        gsl::finally([since{Since{}}] { metricTime.Observe(since.ms()); })};

    OUTCOME_TRY(block_cbor, codec::cbor::encode(block));
    const auto block_cid{CbCid::hash(block_cbor)};
    storage::OneKey key{BytesIn{block_cid}, kv};

    if (key.has()) {
      if (key.getCbor<bool>()) {
        return outcome::success();
      }
      return ERROR_TEXT("validate: block marked as bad");
    }

    if (!block.ticket) {
      return ERROR_TEXT("validate: no ticket");
    }
    if (!block.block_sig) {
      return ERROR_TEXT("validate: no block_sig");
    }
    if (!block.bls_aggregate) {
      return ERROR_TEXT("validate: no bls_aggregate");
    }
    if (!block.miner.isId()) {
      return ERROR_TEXT("validate: miner must be id");
    }
    OUTCOME_TRY(parent, ts_load->load(block.parents));
    std::shared_lock ts_lock{*ts_branches_mutex};
    OUTCOME_TRY(parent_it,
                primitives::tipset::chain::find(branch, parent->height()));
    OUTCOME_TRY(lookback_it,
                getLookbackTipSetForRound(parent_it, block.height));
    OUTCOME_TRY(lookback, ts_load->lazyLoad(lookback_it.second->second));
    OUTCOME_TRY(prev_beacon, latestBeacon(ts_load, parent_it));
    const auto lookback_height{lookback_it.second->first};
    const auto lookback_tsk{lookback_it.second->second.key};
    ts_lock.unlock();
    if (block.height <= parent->height()) {
      return ERROR_TEXT("validate: height less than parent");
    }
    if (block.timestamp
        != parent->getMinTimestamp()
               + (block.height - parent->height()) * kEpochDurationSeconds) {
      return ERROR_TEXT("validate: wrong timestamp");
    }
    OUTCOME_TRY(base_fee, parent->nextBaseFee(ipld));
    if (block.parent_base_fee != base_fee) {
      return ERROR_TEXT("validate: wrong parent_base_fee");
    }
    OUTCOME_TRY(parent_interpreted, interpreter_cache->get(parent->key));
    if (block.parent_weight != parent_interpreted.weight) {
      return ERROR_TEXT("validate: wrong parent_weight");
    }
    if (block.parent_state_root != parent_interpreted.state_root) {
      return ERROR_TEXT("validate: wrong parent_state_root");
    }
    if (block.parent_message_receipts != parent_interpreted.message_receipts) {
      return ERROR_TEXT("validate: wrong parent_message_receipts");
    }
    StateTreeImpl parent_tree{withVersion(ipld, parent->height()),
                              parent_interpreted.state_root};
    OUTCOME_TRY(validateMessages(block, parent, parent_tree));
    OUTCOME_TRY(parent_power_actor, parent_tree.get(kStoragePowerAddress));
    OUTCOME_TRY(parent_power,
                getCbor<PowerActorStatePtr>(parent_tree.getStore(),
                                            parent_power_actor.head));
    OUTCOME_TRY(parent_has_claim, parent_power->hasClaim(block.miner));
    if (!parent_has_claim) {
      return ERROR_TEXT("validate: no claim in parent");
    }
    OUTCOME_TRY(lookback_interpreted, interpreter_cache->get(lookback_tsk));
    StateTreeImpl lookback_tree{withVersion(ipld, lookback_height),
                                lookback_interpreted.state_root};
    OUTCOME_TRY(lookback_miner_actor, lookback_tree.get(block.miner));
    OUTCOME_TRY(lookback_miner,
                getCbor<MinerActorStatePtr>(lookback_tree.getStore(),
                                            lookback_miner_actor.head));
    OUTCOME_TRY(lookback_miner_info, lookback_miner->getInfo());
    OUTCOME_TRY(worker, resolveKey(lookback_tree, lookback_miner_info->worker));
    if (block.election_proof.win_count < 1) {
      return ERROR_TEXT("validate: no win_count");
    }
    OUTCOME_TRY(
        eligible,
        minerEligibleToMine(block.miner, lookback, parent, parent_tree));
    if (!eligible) {
      return ERROR_TEXT("validate: not eligible");
    }
    static const crypto::bls::BlsProviderImpl bls;
    const auto vrf{[&](BytesIn rand, BytesIn vrf) -> outcome::result<void> {
      OUTCOME_TRY(signature, fromSpan<crypto::bls::Signature>(vrf));
      OUTCOME_TRY(
          verified,
          bls.verifySignature(
              rand,
              signature,
              boost::get<primitives::address::BLSPublicKeyHash>(worker.data)));
      if (!verified) {
        return ERROR_TEXT("validate: wrong vrf");
      }
      return outcome::success();
    }};
    const BlockRand rand{
        block.miner,
        block.height,
        block.beacon_entries,
        prev_beacon,
        *parent,
    };
    OUTCOME_TRY(vrf(rand.election, block.election_proof.vrf_proof));
    OUTCOME_TRY(lookback_power_actor, lookback_tree.get(kStoragePowerAddress));
    OUTCOME_TRY(lookback_power,
                getCbor<PowerActorStatePtr>(lookback_tree.getStore(),
                                            lookback_power_actor.head));
    OUTCOME_TRY(lookback_claim, lookback_power->getClaim(block.miner));
    if (block.election_proof.win_count
        != computeWinCount(block.election_proof.vrf_proof,
                           lookback_claim->qa_power,
                           lookback_power->total_qa_power)) {
      return ERROR_TEXT("validate: wrong win_count");
    }
    OUTCOME_TRY(signature_verified, checkBlockSignature(block, worker));
    if (!signature_verified) {
      return ERROR_TEXT("validate: wrong block_sig");
    }
    // TODO(turuslan): verify drand values
    OUTCOME_TRY(vrf(rand.ticket, block.ticket->bytes));
    if (kFakeWinningPost) {
      if (block.win_post_proof.size() != 1
          || block.win_post_proof[0].proof
                 != common::span::cbytes(kFakeWinningPostStr)) {
        return ERROR_TEXT("validate: wrong fake win_post_proof");
      }
    } else {
      OUTCOME_TRY(extended_sectors,
                  getSectorsForWinningPoSt(getNetworkVersion(parent->height()),
                                           block.miner,
                                           lookback_miner,
                                           rand.win));

      std::vector<SectorInfo> sectors;
      sectors.reserve(extended_sectors.size());
      for (const auto &sector : extended_sectors) {
        sectors.emplace_back(primitives::sector::toSectorInfo(sector));
      }

      static proofs::ProofEngineImpl proofs;
      OUTCOME_TRY(win_verified,
                  proofs.verifyWinningPoSt({
                      rand.win,
                      block.win_post_proof,
                      sectors,
                      block.miner.getId(),
                  }));
      if (!win_verified) {
        return ERROR_TEXT("validate: wrong win_post_proof");
      }
    }
    return outcome::success();
  }

  outcome::result<void> BlockValidator::validateMessages(
      const BlockHeader &block, const TipsetCPtr &ts, StateTreeImpl &tree) {
    // TODO(turuslan): verify bls aggregate
    // note: lotus workarounds zero bls bug in block
    // bafy2bzaceapyg2uyzk7vueh3xccxkuwbz3nxewjyguoxvhx77malc2lzn2ybi
    const vm::runtime::Pricelist pricelist{block.height};
    std::map<primitives::address::Address, Nonce> nonces;
    MsgMeta wmeta;
    const auto null_ipld{
        std::make_shared<CbAsAnyIpld>(std::make_shared<NullCbIpld>())};
    cbor_blake::cbLoadT(null_ipld, wmeta);
    const auto network{getNetworkVersion(block.height)};
    const auto matcher{vm::toolchain::Toolchain::createAddressMatcher(network)};
    primitives::GasAmount gas_limit{};
    static const vm::message::MessageSignerImpl signer{
        storage::keystore::kDefaultKeystore};
    auto check{
        [&](const UnsignedMessage &msg, size_t size) -> outcome::result<void> {
          if (!validForBlockInclusion(
                  msg, network, pricelist.onChainMessage(size))) {
            return ERROR_TEXT("validateMessages: validForBlockInclusion");
          }
          gas_limit += msg.gas_limit;
          if (gas_limit > kBlockGasLimit) {
            return ERROR_TEXT("validateMessages: gas limit");
          }
          auto from{msg.from};
          if (network >= NetworkVersion::kVersion13) {
            OUTCOME_TRYA(from, tree.lookupId(from));
          }
          auto nonce{nonces.find(from)};
          if (nonce == nonces.end()) {
            OUTCOME_TRY(actor, tree.get(from));
            if (!matcher->isAccountActor(actor.code)) {
              return ERROR_TEXT("validateMessages: from is not account");
            }
            nonce = nonces.emplace(from, actor.nonce).first;
          }
          if (msg.nonce != nonce->second) {
            return ERROR_TEXT("validateMessages: wrong nonce");
          }
          ++nonce->second;
          return outcome::success();
        }};
    OUTCOME_TRY(rmeta, getCbor<MsgMeta>(ipld, block.messages));
    OUTCOME_TRY(rmeta.bls_messages.visit(
        [&](auto, const CID &cid) -> outcome::result<void> {
          OUTCOME_TRY(cbor, ipld->get(cid));
          OUTCOME_TRY(msg, codec::cbor::decode<UnsignedMessage>(cbor));
          OUTCOME_TRY(check(msg, cbor.size()));
          OUTCOME_TRY(wmeta.bls_messages.append(cid));
          return outcome::success();
        }));
    OUTCOME_TRY(rmeta.secp_messages.visit(
        [&](auto, const CID &cid) -> outcome::result<void> {
          OUTCOME_TRY(cbor, ipld->get(cid));
          OUTCOME_TRY(smsg, codec::cbor::decode<SignedMessage>(cbor));
          if (network >= NetworkVersion::kVersion14 && smsg.signature.isBls()) {
            return ERROR_TEXT("validateMessages: signature is not secp");
          }
          OUTCOME_TRY(check(smsg.message, cbor.size()));
          OUTCOME_TRY(key, resolveKey(tree, smsg.message.from));
          OUTCOME_TRY(signer.verify(key, smsg));
          OUTCOME_TRY(wmeta.secp_messages.append(cid));
          return outcome::success();
        }));
    OUTCOME_TRY(root, setCbor(null_ipld, wmeta));
    if (root != block.messages) {
      return ERROR_TEXT("validateMessages: wrong root");
    }
    return outcome::success();
  }
}  // namespace fc::blockchain::block_validator
