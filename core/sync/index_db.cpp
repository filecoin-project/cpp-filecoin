/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "index_db_backend.hpp"

namespace fc::sync {

  namespace {

    // TODO more fields here
    CBOR_ENCODE_TUPLE(TipsetInfo, key.cids());

  }  // namespace

  IndexDb::IndexDb(KeyValueStoragePtr kv_store,
                   std::shared_ptr<IndexDbBackend> backend)
      : kv_store_(std::move(kv_store)),
        backend_(std::move(backend)),
        cache_(1000, [](const TipsetInfo &info) { return info.key.hash(); }) {}

  outcome::result<void> IndexDb::store(
      TipsetInfoPtr info, const boost::optional<SplitBranch> &branch_rename) {
    common::Buffer hash(info->key.hash());
    if (kv_store_->contains(hash)) {
      return Error::INDEXDB_ALREADY_EXISTS;
    }
    auto tx = backend_->beginTx();
    OUTCOME_TRY(backend_->store(*info, branch_rename));
    OUTCOME_TRY(buffer, codec::cbor::encode(info->key.cids()));
    OUTCOME_TRY(kv_store_->put(hash, std::move(buffer)));
    if (branch_rename) {
      cache_.modifyValues([branch_rename](TipsetInfo &v) {
        if (v.branch == branch_rename->old_id
            && v.height > branch_rename->above_height) {
          v.branch = branch_rename->new_id;
        }
      });
    }
    cache_.put(std::move(info), false);
    tx.commit();
    return outcome::success();
  }

  outcome::result<TipsetInfoCPtr> IndexDb::get(const TipsetHash &hash) {
    TipsetInfoCPtr cached = cache_.get(hash);
    if (cached) {
      return cached;
    }
    OUTCOME_TRY(buffer, kv_store_->get(common::Buffer(hash)));
    OUTCOME_TRY(cids, codec::cbor::decode<std::vector<CID>>(buffer));
    auto key = TipsetKey::create(std::move(cids), hash);
    OUTCOME_TRY(idx, backend_->get(hash));
    auto info = std::make_shared<TipsetInfo>(TipsetInfo{
        std::move(key), idx.branch, idx.height, std::move(idx.parent_hash)});
    cache_.put(info, false);
    return info;
  }

  outcome::result<TipsetInfoCPtr> IndexDb::get(BranchId branch, Height height) {
    OUTCOME_TRY(idx, backend_->get(branch, height));
    TipsetInfoCPtr cached = cache_.get(idx.hash);
    if (cached) {
      return cached;
    }
    OUTCOME_TRY(buffer, kv_store_->get(common::Buffer(idx.hash)));
    OUTCOME_TRY(cids, codec::cbor::decode<std::vector<CID>>(buffer));
    auto key = TipsetKey::create(std::move(cids), idx.hash);
    auto info = std::make_shared<TipsetInfo>(TipsetInfo{
        std::move(key), idx.branch, idx.height, std::move(idx.parent_hash)});
    cache_.put(info, false);
    return info;
  }

}  // namespace fc::sync
