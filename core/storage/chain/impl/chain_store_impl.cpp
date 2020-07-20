/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/chain/impl/chain_store_impl.hpp"

#include "common/logger.hpp"
#include "crypto/randomness/impl/chain_randomness_provider_impl.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/cid/cid_of_cbor.hpp"
#include "primitives/cid/json_codec.hpp"
#include "primitives/tipset/tipset.hpp"
#include "storage/chain/datastore_key.hpp"
#include "storage/chain/impl/chain_data_store_impl.hpp"

namespace fc::storage::blockchain {

  using crypto::randomness::ChainRandomnessProvider;
  using crypto::randomness::ChainRandomnessProviderImpl;
  using primitives::address::Address;
  //  using primitives::block::BlockHeader;
  using primitives::tipset::Tipset;
  //  using primitives::tipset::TipsetKey;

  using primitives::BigInt;

  //
  //  /** functions */
  //  using codec::json::decodeCidVector;
  //  using codec::json::encodeCidVector;
  //  using primitives::cid::getCidOfCbor;

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("chainstore");
      return logger.get();
    }

    // const DatastoreKey kChainHeadKey{DatastoreKey::makeFromString("head")};
    // const DatastoreKey kGenesisKey{DatastoreKey::makeFromString("0")};
  }  // namespace

  ChainStoreImpl::ChainStoreImpl(
      std::shared_ptr<IpfsDatastore> data_store,
      std::shared_ptr<BlockValidator> block_validator,
      std::shared_ptr<WeightCalculator> weight_calculator,
      std::shared_ptr<storage::indexdb::IndexDb> index_db,
      std::shared_ptr<sync::TipsetLoader> tipset_loader,
      std::shared_ptr<sync::PeerManager> peer_manager)
      : data_store_{std::move(data_store)},
        block_validator_{std::move(block_validator)},
        weight_calculator_{std::move(weight_calculator)},
        index_db_(std::move(index_db)),
        tipset_loader_(std::move(tipset_loader)),
        peer_manager_(std::move(peer_manager)) {
    // TODO assert
  }

  outcome::result<void> ChainStoreImpl::init(BlockHeader genesis_header) {
    OUTCOME_TRYA(genesis_, Tipset::create({std::move(genesis_header)}));

    heads_ = index_db_->getHeads();
    if (heads_.empty()) {
      // we are launching empty node from scratch
      OUTCOME_TRY(index_db_->writeGenesis(genesis_.value()));
      heads_ = index_db_->getHeads();
      if (heads_.size() != 1) {
        return ChainStoreError::DATA_INTEGRITY_ERROR;
      }

      // TODO verify fields

      heaviest_tipset_ = genesis_;

      OUTCOME_TRYA(
          heaviest_weight_,
          weight_calculator_->calculateWeight(heaviest_tipset_.value()));
    } else {
      // TODO verify genesis tipset hash

      OUTCOME_TRY(chooseHead());
    }

    // TODO transfer actual head to peer manager

    return outcome::success();
  }

  outcome::result<void> ChainStoreImpl::chooseHead() {
    using namespace storage::indexdb;

    if (heads_.empty()) {
      return ChainStoreError::DATA_INTEGRITY_ERROR;
    }

    BigInt max_weight;
    size_t idx = 0;
    size_t head_idx = 0;
    Tipset head_tipset;
    for (size_t sz = heads_.size(); idx < sz; ++idx) {
      const auto &head = heads_[idx].get();
      if (head.root != kGenesisBranch) {
        // this is not yet synced branch, not suitable to be the head
        continue;
      }

      OUTCOME_TRY(tipset, loadTipsetLocally(head.top, {}));
      OUTCOME_TRY(weight, weight_calculator_->calculateWeight(head_tipset));
      if (weight > max_weight) {
        max_weight = weight;
        head_idx = idx;
        head_tipset = std::move(tipset);
      }
    }

    // at least 1 branch must originate from genesis
    if (idx >= heads_.size()) {
      // TODO logs everywhere
      return ChainStoreError::DATA_INTEGRITY_ERROR;
    }

    heaviest_weight_ = std::move(max_weight);
    heaviest_tipset_ = std::move(head_tipset);

    return outcome::success();
  }

  outcome::result<Tipset> ChainStoreImpl::loadTipsetLocally(
      const TipsetHash &hash, const std::vector<CID> &cids) {
    const std::vector<CID> *cids_ptr;
    std::vector<CID> cids_loaded;
    if (!cids.empty()) {
      cids_ptr = &cids;
    } else {
      OUTCOME_TRYA(cids_loaded, index_db_->getTipsetCIDs(hash));
      cids_ptr = &cids_loaded;
    }
    OUTCOME_TRY(tipset, Tipset::load(*data_store_, *cids_ptr));
    return std::move(tipset);
  }

  outcome::result<Tipset> ChainStoreImpl::loadTipset(const TipsetKey &key) {
    // TODO cache, load & hash, optimize
    return Tipset::load(*data_store_, key.cids());
  }

  void ChainStoreImpl::onTipsetAsyncLoaded(TipsetHash hash,
                                           outcome::result<Tipset> tipset_res) {
    if (!tipset_res) {
      // TODO analyze error & choose retry policy, another peer, ...
      return;
    }

    const auto &tipset = tipset_res.value();
    if (tipset_in_sync_ && tipset.key.hash() == tipset_in_sync_.value()) {
      auto parent_res = tipset.getParents();
      if (!parent_res) {
        // TODO handle this, syncing failed

        tipset_in_sync_ = boost::none;
        syncing_branch_ = storage::indexdb::kNoBranch;
        return;
      }
/* TODO
      auto index_res = index_db_->applyToBottom(tipset, syncing_branch_);
      if (!index_res) {
        log()->error("indexing tipset failed, {}", index_res.error().message());
        return;
      }

      const auto &parent_hash = parent_res.value().hash();
      if (parent_hash == heaviest_tipset_.value().key.hash()) {
        // syncing is finished
        index_res = index_db_->linkBranches(head_branch_.id, syncing_branch_);
        if (!index_res) {
          log()->error("sync branches index failed, {}",
                       index_res.error().message());
          return;
        }

        onSyncFinished(outcome::success());
      } else {
        synchronizeTipset(std::move(parent_res.value()));
      }
    */
    }

  }

  void ChainStoreImpl::onSyncFinished(outcome::result<void> result) {
    if (result) {
      // apply new head step by step
      std::error_code e;
      result = index_db_->loadChain(
          heaviest_tipset_->key.hash(),
          [this, &e](TipsetHash hash, std::vector<CID> cids) {
            if (!e) {
              auto res = applyHead(std::move(hash), std::move(cids));
              if (!res) {
                e = res.error();
              }
            }
          });

      if (e) {
        result = e;
      }
    }

    if (result) {
      // reload head branches
      auto res = index_db_->getBranchInfo(head_branch_.id);
      if (!res) {
        result = res.error();
      }
      heads_ = index_db_->getHeads();
    }

    if (!result) {
      log()->error("sync failed, {}", result.error().message());

      // TODO handle this
    }

    tipset_in_sync_ = boost::none;
    syncing_branch_ = storage::indexdb::kNoBranch;
  }

  void ChainStoreImpl::synchronizeTipset(TipsetKey key) {
    if (!current_peer_) {
      choosePeer();
    }
    auto res = tipset_loader_->loadTipsetAsync(key, current_peer_);
    if (!res) {
      onSyncFinished(std::move(res));
      return;
    }
    tipset_in_sync_ = key.hash();
    if (syncing_branch_ == storage::indexdb::kNoBranch) {
      syncing_branch_ = index_db_->getNewBranchId();
    }
  }

  void ChainStoreImpl::choosePeer() {
    // TODO call peer_manager_ for most weighted peer
  }

  outcome::result<void> ChainStoreImpl::applyHead(TipsetHash hash,
                                                  std::vector<CID> cids) {
    OUTCOME_TRY(tipset, loadTipsetLocally(hash, cids));
    OUTCOME_TRY(weight, weight_calculator_->calculateWeight(tipset));

    // TODO check if it's possible in weight calculator
    //    if (weight <= heaviest_weight_) {
    assert(weight >= heaviest_weight_);

    heaviest_weight_ = std::move(weight);
    heaviest_tipset_ = tipset;
    head_change_signal_(
        HeadChange{.type = HeadChangeType::APPLY, .value = std::move(tipset)});

    // TODO say to hello about new head
    // TODO publish into gossip

    return outcome::success();
  }

    outcome::result<Tipset> ChainStoreImpl::heaviestTipset() const {
      if (heaviest_tipset_.has_value()) {
        return *heaviest_tipset_;
      }
      return ChainStoreError::NO_HEAVIEST_TIPSET;
    }

  //  outcome::result<bool> ChainStoreImpl::containsTipset(
  //      const TipsetKey &key) const {
  //    if (tipsets_cache_.count(key) > 0) {
  //      return true;
  //    }
  //
  //    OUTCOME_TRY(loadTipset(key));
  //    return true;
  //  }

//    outcome::result<BlockHeader> ChainStoreImpl::getGenesis() const {
//      if (genesis_.has_value() && !genesis_->blks.empty()) {
//        return genesis_->blks[0];
//      }
//
//      return ChainStoreError::NO_GENESIS_BLOCK;
//    }

  outcome::result<TipsetKey> ChainStoreImpl::genesisTipsetKey() const {
    if (genesis_.has_value() && !genesis_->blks.empty()) {
      return genesis_->key;
    }

    return ChainStoreError::NO_GENESIS_BLOCK;
  }

  //  outcome::result<void> ChainStoreImpl::writeGenesis(
  //      const BlockHeader &block_header) {
  //    OUTCOME_TRY(genesis_tipset, Tipset::create({block_header}));
  //    OUTCOME_TRY(
  //        buffer,
  //        encodeCidVector(std::vector<CID>{genesis_tipset.key.cids()[0]}));
  //
  //    return chain_data_store_->set(kGenesisKey, buffer);
  //  }
  //
  //  outcome::result<void> ChainStoreImpl::writeHead(const Tipset &tipset) {
  //    heaviest_tipset_.reset(tipset);
  //    OUTCOME_TRY(weight, weight_calculator_->calculateWeight(tipset));
  //    heaviest_weight_ = weight;
  //    OUTCOME_TRY(data, encodeCidVector(tipset.key.cids()));
  //    return chain_data_store_->set(kChainHeadKey, data);
  //  }
  //
  //  outcome::result<void> ChainStoreImpl::addBlock(const BlockHeader &block) {
  //    OUTCOME_TRY(data_store_->setCbor(block));
  //    OUTCOME_TRY(tipset, expandTipset(block));
  //    return updateHeaviestTipset(tipset);
  //  }
  //
  //  outcome::result<Tipset> ChainStoreImpl::expandTipset(
  //      const BlockHeader &block_header) {
  //    std::vector<BlockHeader> all_headers{block_header};
  //
  //    if (tipsets_.find(block_header.height) == std::end(tipsets_)) {
  //      return Tipset::create(all_headers);
  //    }
  //    auto &&tipsets = tipsets_[block_header.height];
  //
  //    std::set<Address> included_miners;
  //    included_miners.insert(block_header.miner);
  //
  //    OUTCOME_TRY(block_cid, getCidOfCbor(block_header));
  //    for (auto &&cid : tipsets) {
  //      if (cid == block_cid) {
  //        continue;
  //      }
  //
  //      OUTCOME_TRY(bh, getCbor<BlockHeader>(cid));
  //
  //      if (included_miners.count(bh.miner) > 0) {
  //        auto &&miner_address = encodeToString(bh.miner);
  //        logger_->warn(
  //            "Have multiple blocks from miner {} at height {} in our tipset "
  //            "cache",
  //            miner_address,
  //            bh.height);
  //        continue;
  //      }
  //
  //      if (bh.parents == block_header.parents) {
  //        all_headers.push_back(bh);
  //        included_miners.insert(bh.miner);
  //      }
  //    }
  //
  //    return Tipset::create(all_headers);
  //  }
  //
  //  outcome::result<void> ChainStoreImpl::updateHeaviestTipset(
  //      const Tipset &tipset) {
  //    OUTCOME_TRY(weight, weight_calculator_->calculateWeight(tipset));
  //
  //    if (!heaviest_tipset_.has_value()) {
  //      return takeHeaviestTipset(tipset);
  //    }
  //
  //    OUTCOME_TRY(current_weight,
  //                weight_calculator_->calculateWeight(*heaviest_tipset_));
  //
  //    if (weight > current_weight) {
  //      return takeHeaviestTipset(tipset);
  //    }
  //
  //    return outcome::success();
  //  }
  //
  //  outcome::result<void> ChainStoreImpl::takeHeaviestTipset(
  //      const Tipset &tipset) {
  //    OUTCOME_TRY(cids_json, encodeCidVector(tipset.key.cids()));
  //
  //    if (!heaviest_tipset_.has_value()) {
  //      logger_->warn(
  //          "No heaviest tipset found, using provided tipset: {}, height: {}",
  //          cids_json,
  //          tipset.height());
  //      head_change_signal_(
  //          HeadChange{.type = HeadChangeType::CURRENT, .value = tipset});
  //      return writeHead(tipset);
  //    }
  //
  //    if (tipset == *heaviest_tipset_) {
  //      logger_->warn(
  //          "provided tipset is equal to the current one, nothing happens");
  //      return outcome::success();
  //    }
  //
  //    logger_->info(
  //        "New heaviest tipset {} (height={})", cids_json, tipset.height());
  //
  //    OUTCOME_TRY(notifyHeadChange(*heaviest_tipset_, tipset));
  //    OUTCOME_TRY(writeHead(tipset));
  //
  //    return outcome::success();
  //  }
  //
  //  outcome::result<void> ChainStoreImpl::notifyHeadChange(const Tipset
  //  &current,
  //                                                         const Tipset
  //                                                         &target) {
  //    OUTCOME_TRY(path, findChainPath(current, target));
  //
  //    for (auto &revert_item : path.revert_chain) {
  //      head_change_signal_(
  //          HeadChange{.type = HeadChangeType::REVERT, .value = revert_item});
  //    }
  //
  //    for (auto &apply_item : path.apply_chain) {
  //      head_change_signal_(
  //          HeadChange{.type = HeadChangeType::APPLY, .value = apply_item});
  //    }
  //
  //    return outcome::success();
  //  }
  //
    std::shared_ptr<ChainRandomnessProvider>
    ChainStoreImpl::createRandomnessProvider() {
      return
      std::make_shared<ChainRandomnessProviderImpl>(shared_from_this());
    }
  //
  //  outcome::result<ChainPath> ChainStoreImpl::findChainPath(
  //      const Tipset &current, const Tipset &target) {
  //    // need to have genesis defined
  //    ChainPath path{};
  //    auto l = current;
  //    auto r = target;
  //    while (l != r) {
  //      if (l.height() > r.height()) {
  //        path.revert_chain.emplace_back(l);
  //        OUTCOME_TRY(key, l.getParents());
  //        OUTCOME_TRY(ts, loadTipset(key));
  //        l = std::move(ts);
  //      } else {
  //        path.apply_chain.emplace_front(r);
  //        OUTCOME_TRY(key, r.getParents());
  //        OUTCOME_TRY(ts, loadTipset(key));
  //        r = std::move(ts);
  //      }
  //    }
  //    return path;
  //  }

}  // namespace fc::storage::blockchain

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::blockchain, ChainStoreError, e) {
  using fc::storage::blockchain::ChainStoreError;

  switch (e) {
    case ChainStoreError::NO_MIN_TICKET_BLOCK:
      return "min ticket block has no value";
    case ChainStoreError::NO_HEAVIEST_TIPSET:
      return "no heaviest tipset in storage";
    case ChainStoreError::NO_GENESIS_BLOCK:
      return "no genesis block in storage";
    case ChainStoreError::STORE_NOT_INITIALIZED:
      return "store is not initialized properly";
    case ChainStoreError::DATA_INTEGRITY_ERROR:
      return "chain store data integrity error";
  }

  return "ChainStoreError: unknown error";
}
