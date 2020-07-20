/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tipset_graph.hpp"

namespace fc::sync {

  TipsetCache createTipsetCache(size_t max_size) {
    return TipsetCache(max_size,
                       [](const Tipset &tipset) { return tipset.key.hash(); });
  }

  ChainDb::ChainDb()
      : state_error_(NOT_INITIALIZED), tipset_cache_(createTipsetCache(1000)) {}

  outcome::result<void> ChainDb::init(KeyValueStoragePtr key_value_storage,
                                      IpfsStoragePtr ipfs_storage,
                                      std::shared_ptr<IndexDb> index_db,
                                      const std::string &load_car_path) {
    assert(key_value_storage);
    assert(ipfs_storage);
    assert(index_db);

    try {
      // load car
      // load genesis tipset
      // load branches

      state_error_.clear();
    } catch (const std::system_error &e) {
      return e.code();
    }

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

  const std::string &ChainDb::networkName() const {
    OUTCOME_EXCEPT(stateIsConsistent());
    return network_name_;
  }

  outcome::result<void> ChainDb::getHeads(const HeadCallback &callback) {
    assert(callback);
    OUTCOME_TRY(stateIsConsistent());
    auto heads = branches_.getAllHeads();
    for (const auto &[hash, _] : heads) {
      // TODO OUTCOME_TRY(tipset, getTipsetByHash(hash));
      callback(boost::none, hash);
    }
    return outcome::success();
  }

  outcome::result<TipsetCPtr> ChainDb::getTipsetByHash(const TipsetHash &hash) {
    OUTCOME_TRY(stateIsConsistent());
    TipsetCPtr tipset = tipset_cache_.get(hash);
    if (tipset) {
      return tipset;
    }
    OUTCOME_TRY(info, index_db_->get(hash));
    auto sptr = std::make_shared<Tipset>();
    OUTCOME_TRYA(*sptr, Tipset::load(*ipfs_storage_, info->key.cids()));
    tipset_cache_.put(sptr, false);
    return sptr;
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
    auto internal_cb = [&, this](const TipsetInfo &info) {
      if (!e) {
        auto res = getTipsetByHash(info.key.hash());
        if (res) {
          last_height = info.height;
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
      from_height = last_height;
    }

    return e;
  }

  outcome::result<void> ChainDb::walkBackward(const TipsetHash &from,
                                              Height to_height,
                                              const WalkCallback &cb) {
    OUTCOME_TRY(stateIsConsistent());

    std::error_code e;
    auto internal_cb = [this, &cb, &e](const TipsetInfo &info) {
      if (!e) {
        auto res = getTipsetByHash(info.key.hash());
        if (res) {
          cb(std::move(res.value()));
        } else {
          e = res.error();
        }
      }
    };

    OUTCOME_TRY(index_db_->walkBackward(from, to_height, internal_cb));
    return e;
  }

  outcome::result<void> ChainDb::storeTipset(
      std::shared_ptr<Tipset> tipset,
      const TipsetHash &parent_hash,
      const HeadCallback &on_heads_changed) {
    OUTCOME_TRY(stateIsConsistent());

    assert(on_heads_changed);
    assert(tipset);

    if (tipsetIsStored(tipset->key.hash())) {
      return outcome::success();
    }
    if (tipset->height() == 0) {
      return Error::TIPSET_IS_BAD;
    }
    if ((tipset->height() == 1)
        && (parent_hash != genesis_tipset_->key.hash())) {
      return Error::TIPSET_IS_BAD;
    }

    bool parent_must_exist = tipsetIsStored(parent_hash);
    BranchId parent_branch = kNoBranch;
    Height parent_height = 0;
    if (parent_must_exist) {
      OUTCOME_TRY(info, index_db_->get(parent_hash));
      parent_branch = info->branch;
      parent_height = info->height;
    }

    OUTCOME_TRY(store_position,
                branches_.findStorePosition(
                    *tipset, parent_hash, parent_branch, parent_height));

    auto info =
        std::make_shared<TipsetInfo>(TipsetInfo{tipset->key,
                                                store_position.assigned_branch,
                                                tipset->height(),
                                                parent_hash});

    OUTCOME_TRY(index_db_->store(std::move(info), store_position.split));

    tipset_cache_.put(tipset, false);

    auto head_changes =
        branches_.storeTipset(tipset, parent_hash, store_position);

    for (auto& change : head_changes) {
      on_heads_changed(std::move(change.removed), std::move(change.added));
    }

    return outcome::success();
  }

}  // namespace fc::sync
