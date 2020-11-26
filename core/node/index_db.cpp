/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <algorithm>

#include "index_db_backend.hpp"

#include "common/logger.hpp"

namespace fc::sync {

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("indexdb");
      return logger.get();
    }
  }  // namespace

  IndexDb::IndexDb(std::shared_ptr<IndexDbBackend> backend)
      : backend_(std::move(backend)),
        cache_(1000, [](const TipsetInfo &info) { return info.key.hash(); }) {}

  outcome::result<std::map<BranchId, std::shared_ptr<BranchInfo>>>
  IndexDb::init() {
    return backend_->initDb();
  }

  outcome::result<void> IndexDb::storeGenesis(const Tipset &genesis_tipset) {
    auto info = std::make_shared<TipsetInfo>(
        TipsetInfo{genesis_tipset.key, kGenesisBranch, 0, {}});
    return store(std::move(info), boost::none);
  }

  outcome::result<void> IndexDb::store(
      TipsetInfoPtr info, const boost::optional<RenameBranch> &branch_rename) {
    common::Buffer hash(info->key.hash());

    log()->debug("store: {}:{}:{}",
                 info->height,
                 info->branch,
                 info->key.toPrettyString());

    auto tx = backend_->beginTx();
    OUTCOME_TRY(backend_->store(*info, branch_rename));
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

  bool IndexDb::contains(const TipsetHash &hash) {
    auto res = get(hash);
    return res.has_value();
  }

  outcome::result<TipsetInfoCPtr> IndexDb::get(const TipsetHash &hash) {
    TipsetInfoCPtr cached = cache_.get(hash);
    if (cached) {
      return cached;
    }
    OUTCOME_TRY(idx, backend_->get(hash));
    OUTCOME_TRY(info, IndexDbBackend::decode(std::move(idx)));

    cache_.put(info, false);

    log()->debug("get: {}:{}", info->height, info->key.toPrettyString());

    return std::move(info);
  }

  outcome::result<TipsetInfoCPtr> IndexDb::get(BranchId branch, Height height) {
    OUTCOME_TRY(idx, backend_->get(branch, height));
    TipsetInfoCPtr cached = cache_.get(idx.hash);
    if (cached) {
      return cached;
    }
    OUTCOME_TRY(info, IndexDbBackend::decode(std::move(idx)));

    cache_.put(info, false);

    log()->debug("get: {}:{}", info->height, info->key.toPrettyString());

    return std::move(info);
  }

  outcome::result<void> IndexDb::walkForward(BranchId branch,
                                             Height from_height,
                                             Height to_height,
                                             size_t limit,
                                             const WalkCallback &cb) {
    if (to_height < from_height || limit == 0) {
      return outcome::success();
    }
    auto diff = to_height - from_height + 1;
    limit = std::min(limit, diff);
    std::error_code e;
    auto res = backend_->walk(
        branch,
        from_height,
        limit,
        [&e, &cb, to_height](IndexDbBackend::TipsetIdx raw) {
          if (!e && raw.height <= to_height) {
            auto decode_res = IndexDbBackend::decode(std::move(raw));
            if (!decode_res) {
              e = decode_res.error();
            } else {
              cb(std::move(decode_res.value()));
            }
          }
        });
    if (!res) {
      return res.error();
    }
    if (e) {
      return e;
    }
    return outcome::success();
  }

  outcome::result<void> IndexDb::walkBackward(const TipsetHash &from,
                                              Height to_height,
                                              const WalkCallback &cb) {
    TipsetInfoCPtr info;
    OUTCOME_TRYA(info, get(from));
    for (;;) {
      OUTCOME_TRYA(info, get(info->parent_hash));
      if (info->height <= to_height) {
        break;
      }
      cb(info);
    }
    return outcome::success();
  }

}  // namespace fc::sync
