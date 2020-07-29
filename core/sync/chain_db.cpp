/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "chain_db.hpp"

namespace fc::sync {

  namespace {
    TipsetCache createTipsetCache(size_t max_size) {
      return TipsetCache(
          max_size, [](const Tipset &tipset) { return tipset.key.hash(); });
    }

    auto log() {
      static common::Logger logger = common::createLogger("chaindb");
      return logger.get();
    }
  }  // namespace

  ChainDb::ChainDb()
      : state_error_(Error::SYNC_NOT_INITIALIZED),
        tipset_cache_(createTipsetCache(1000)) {}

  outcome::result<void> ChainDb::init(KeyValueStoragePtr key_value_storage,
                                      IpfsStoragePtr ipld,
                                      std::shared_ptr<IndexDb> index_db,
                                      const boost::optional<CID> &genesis_cid,
                                      bool creating_new_db) {
    assert(key_value_storage);
    assert(ipld);
    assert(index_db);

    key_value_storage_ = std::move(key_value_storage);
    ipld_ = std::move(ipld);
    index_db_ = std::move(index_db);

    try {
      OUTCOME_EXCEPT(branches_map, index_db_->init());

      state_error_.clear();

      if (creating_new_db) {
        if (!genesis_cid) {
          throw std::system_error(Error::SYNC_NO_GENESIS);
        }
        if (!branches_map.empty()) {
          throw std::system_error(Error::SYNC_DATA_INTEGRITY_ERROR);
        }
        OUTCOME_EXCEPT(gt, Tipset::loadGenesis(*ipld_, genesis_cid.value()));

        assert(gt.key.cids()[0] == genesis_cid.value());

        genesis_tipset_ = std::make_shared<Tipset>(std::move(gt));
        OUTCOME_EXCEPT(branches_.storeGenesis(genesis_tipset_));
        OUTCOME_EXCEPT(index_db_->storeGenesis(*genesis_tipset_));
      } else {
        if (branches_map.empty()) {
          throw std::system_error(Error::SYNC_NO_GENESIS);
        }
        OUTCOME_EXCEPT(branches_.init(std::move(branches_map)));
        OUTCOME_EXCEPT(info, index_db_->get(kGenesisBranch, 0));
        genesis_tipset_ = std::make_shared<Tipset>();
        if (genesis_cid) {
          if (genesis_cid.value() != info->key.cids()[0]) {
            throw std::system_error(Error::SYNC_GENESIS_MISMATCH);
          }
        }
        OUTCOME_TRYA(*genesis_tipset_,
                     Tipset::loadGenesis(*ipld_, info->key.cids()[0]));
      }

    } catch (const std::system_error &e) {
      state_error_ = e.code();
      return e.code();
    }

    return outcome::success();
  }

  outcome::result<void> ChainDb::start(HeadCallback on_heads_changed) {
    assert(on_heads_changed);
    OUTCOME_TRY(stateIsConsistent());
    head_callback_ = std::move(on_heads_changed);
    started_ = true;
    return outcome::success();
  }

  outcome::result<void> ChainDb::stateIsConsistent() const {
    if (!state_error_) {
      return outcome::success();
    }
    return state_error_;
  }

  const CID &ChainDb::genesisCID() const {
    OUTCOME_EXCEPT(stateIsConsistent());
    assert(genesis_tipset_);
    return genesis_tipset_->key.cids()[0];
  }

  const Tipset &ChainDb::genesisTipset() const {
    OUTCOME_EXCEPT(stateIsConsistent());
    assert(genesis_tipset_);
    return *genesis_tipset_;
  }

  bool ChainDb::tipsetIsStored(const TipsetHash &hash) const {
    OUTCOME_EXCEPT(stateIsConsistent());

    return index_db_->contains(hash);

    //TODO revert back after restoring db
    // return key_value_storage_->contains(common::Buffer(hash));
  }

  outcome::result<void> ChainDb::getHeads(const HeadCallback &callback) {
    assert(callback);
    OUTCOME_TRY(stateIsConsistent());
    auto heads = branches_.getAllHeads();
    for (const auto &[hash, info] : heads) {
      if (info->synced_to_genesis) {
        callback(boost::none, hash);
      }
    }
    return outcome::success();
  }

  outcome::result<TipsetCPtr> ChainDb::getTipsetByHash(const TipsetHash &hash) {
    OUTCOME_TRY(stateIsConsistent());
    if (hash == genesis_tipset_->key.hash()) {
      // special case due to tickets and loading
      return genesis_tipset_;
    }
    TipsetCPtr tipset = tipset_cache_.get(hash);
    if (tipset) {
      return tipset;
    }
    OUTCOME_TRY(info, index_db_->get(hash));
    auto sptr = std::make_shared<Tipset>();
    OUTCOME_TRYA(*sptr, Tipset::load(*ipld_, info->key.cids()));
    tipset_cache_.put(sptr, false);
    return sptr;
  }

  outcome::result<void> ChainDb::setCurrentHead(const TipsetHash &head) {
    OUTCOME_TRY(stateIsConsistent());
    OUTCOME_TRY(info, index_db_->get(head));
    return branches_.setCurrentHead(info->branch, info->height);
  }

  outcome::result<TipsetCPtr> ChainDb::getTipsetByHeight(Height height) {
    OUTCOME_TRY(stateIsConsistent());
    OUTCOME_TRY(branch_id, branches_.getBranchAtHeight(height, true));
    OUTCOME_TRY(info, index_db_->get(branch_id, height));
    return getTipsetByHash(info->key.hash());
  }

  outcome::result<void> ChainDb::walkForward(Height from_height,
                                             Height to_height,
                                             const WalkCallback &cb) {
    OUTCOME_TRY(stateIsConsistent());

    Height last_height = 0;
    std::error_code e;
    auto internal_cb = [&, this](const TipsetHash &hash,
                                 BranchId branch,
                                 Height height,
                                 const TipsetHash &parent_hash) {
      if (!e) {
        auto res = getTipsetByHash(hash);
        if (res) {
          last_height = height;
          cb(std::move(res.value()));
        } else {
          e = res.error();
        }
      }
    };

    for (;;) {
      OUTCOME_TRY(branch_id, branches_.getBranchAtHeight(from_height, false));
      if (branch_id == kNoBranch || last_height >= to_height) {
        break;
      }
      OUTCOME_TRY(index_db_->walkForward(
          branch_id, from_height, to_height, internal_cb));
      if (e) {
        break;
      }
      from_height = last_height + 1;
    }

    if (e) {
      return e;
    }
    return outcome::success();
  }

  outcome::result<void> ChainDb::walkBackward(const TipsetHash &from,
                                              Height to_height,
                                              const WalkCallback &cb) {
    OUTCOME_TRY(stateIsConsistent());

    TipsetHash h = from;
    for (;;) {
      OUTCOME_TRY(tipset, getTipsetByHash(h));
      auto height = tipset->height();
      if (height > 0) {
        OUTCOME_TRY(parent_key, tipset->getParents());
        h = parent_key.hash();
      }
      cb(std::move(tipset));
      if (height <= to_height) {
        break;
      }
    }
    return outcome::success();
  }

  outcome::result<boost::optional<TipsetCPtr>> ChainDb::storeTipset(
      std::shared_ptr<Tipset> tipset, const TipsetKey &parent) {
    OUTCOME_TRY(stateIsConsistent());
    if (!started_) {
      return Error::SYNC_NOT_INITIALIZED;
    }

    assert(tipset);

    if (tipsetIsStored(tipset->key.hash())) {
      return getUnsyncedBottom(tipset->key);
    }
    if (tipset->height() == 0) {
      return Error::SYNC_BAD_TIPSET;
    }
    if ((tipset->height() == 1)
        && (parent.hash() != genesis_tipset_->key.hash())) {
      return Error::SYNC_BAD_TIPSET;
    }

    bool parent_must_exist = tipsetIsStored(parent.hash());
    BranchId parent_branch = kNoBranch;
    Height parent_height = 0;
    if (parent_must_exist) {
      OUTCOME_TRY(info, index_db_->get(parent.hash()));
      parent_branch = info->branch;
      parent_height = info->height;
    }

    OUTCOME_TRY(store_position,
                branches_.findStorePosition(
                    *tipset, parent.hash(), parent_branch, parent_height));

    if (store_position.rename && store_position.rename.value().split) {
      auto &split = store_position.rename.value();
      assert(parent_height == split.above_height);
      assert(parent_branch == split.old_id);
      OUTCOME_TRY(new_bottom_info,
                  index_db_->get(parent_branch, parent_height + 1));

      assert(new_bottom_info->parent_hash == parent.hash());

      branches_.splitBranch(parent.hash(),
                            new_bottom_info->key.hash(),
                            new_bottom_info->height,
                            split);
    }

    auto info =
        std::make_shared<TipsetInfo>(TipsetInfo{tipset->key,
                                                store_position.assigned_branch,
                                                tipset->height(),
                                                parent.hash()});

    OUTCOME_TRY(index_db_->store(std::move(info), store_position.rename));

    tipset_cache_.put(tipset, false);

    auto head_changes =
        branches_.storeTipset(tipset, parent.hash(), store_position);

    if (head_changes.empty()) {
      // no heads appeared, this branch is unsynced
      if (store_position.at_bottom_of_branch
          == store_position.assigned_branch) {
        return TipsetCPtr(tipset);
      }

      // need to search for bottom
      OUTCOME_TRY(branch_info,
                  branches_.getRootBranch(store_position.assigned_branch));
      if (branch_info->id != kGenesisBranch) {
        OUTCOME_TRY(last_loaded, getTipsetByHash(branch_info->bottom));
        return std::move(last_loaded);
      }

    } else {
      for (auto &change : head_changes) {
        head_callback_(std::move(change.removed), std::move(change.added));
      }
    }

    return boost::none;
  }

  outcome::result<boost::optional<TipsetCPtr>> ChainDb::getUnsyncedBottom(
      const TipsetKey &key) {
    OUTCOME_TRY(tipset_info, index_db_->get(key.hash()));
    OUTCOME_TRY(branch_info, branches_.getRootBranch(tipset_info->branch));
    if (branch_info->id != kGenesisBranch) {
      OUTCOME_TRY(last_loaded, getTipsetByHash(branch_info->bottom));
      return std::move(last_loaded);
    }
    return boost::none;
  }

}  // namespace fc::sync
