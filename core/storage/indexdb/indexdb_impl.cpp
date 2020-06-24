/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "indexdb_impl.hpp"

#include "codec/cbor/cbor.hpp"

namespace fc::storage::indexdb {

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger(kLogName);
      return logger.get();
    }
  }  // namespace

  Branches IndexDbImpl::getHeads() {
    return graph_.getHeads();
  }

  BranchId IndexDbImpl::getNewBranchId() {
    return ++branch_id_counter_;
  }

  outcome::result<std::reference_wrapper<const BranchInfo>>
  IndexDbImpl::getBranchInfo(BranchId id) {
    return graph_.getBranch(id);
  }

  outcome::result<TipsetInfo> IndexDbImpl::getTipsetInfo(
      const TipsetHash &hash) {
    TipsetInfo info;
    auto cb = [&info](Blob hash,
                      BranchId branch,
                      Height height,
                      Blob parent_hash,
                      BranchId parent_branch) {
      info.hash = std::move(hash);
      info.branch = branch;
      info.height = height;
      info.parent_hash = std::move(parent_hash);
      info.parent_branch = parent_branch;
    };
    bool res = db_.execQuery(get_tipset_info_, cb, hash);
    if (!res) {
      return Error::INDEXDB_EXECUTE_ERROR;
    }
    if (info.hash.empty()) {
      return Error::TIPSET_NOT_FOUND;
    }
    return info;
  }

  outcome::result<std::vector<CID>> IndexDbImpl::getTipsetCIDs(
      const TipsetHash &hash) {
    Blob blob;
    auto cb = [&blob](Blob blocks) { blob = std::move(blocks); };
    bool res = db_.execQuery(get_tipset_blocks_, cb, hash);
    if (!res) {
      return Error::INDEXDB_EXECUTE_ERROR;
    }
    if (blob.empty()) {
      return Error::TIPSET_NOT_FOUND;
    }
    auto decode_res = codec::cbor::decode<std::vector<CID>>(blob);
    if (!decode_res) {
      log()->error("getTipsetCIDs: {}", decode_res.error().message());
      return Error::INDEXDB_DECODE_ERROR;
    }
    return decode_res.value();
  }

  outcome::result<void> IndexDbImpl::appendTipsetOnTop(const Tipset &tipset,
                                                       // ???const TipsetHash& parent,
                                                       BranchId branchId) {
    OUTCOME_TRY(b, graph_.getBranch(branchId));
    const auto &branch_info = b.get();
    if (!branch_info.forks.empty()) {
      return Error::BRANCH_IS_NOT_A_HEAD;
    }

    if (tipset.height() <= branch_info.top_height) {
      return Error::LINK_HEIGHT_MISMATCH;
    }

    OUTCOME_TRY(parent_key, tipset.getParents());
    if (parent_key.hash() != branch_info.top) {
      return Error::UNEXPECTED_TIPSET_PARENT;
    }

    OUTCOME_TRY(buffer, codec::cbor::encode(tipset.key.cids()));

    auto tx = beginTx();

    int rows = db_.execCommand(insert_tipset_,
                               tipset.key.hash(),
                               branchId,
                               tipset.height(),
                               branch_info.top,
                               branchId,
                               buffer.toVector());

    if (rows != 1) {
      return Error::INDEXDB_EXECUTE_ERROR;
    }

    OUTCOME_TRY(
        graph_.appendToHead(branchId, tipset.key.hash(), tipset.height()));

    tx.commit();
    return outcome::success();
  }

  IndexDbImpl::Tx::Tx(IndexDbImpl &db) : db_(db) {
    db_.db_ << "begin";
  }

  void IndexDbImpl::Tx::commit() {
    if (!done_) {
      done_ = true;
      db_.db_ << "commit";
    }
  }

  void IndexDbImpl::Tx::rollback() {
    if (!done_) {
      done_ = true;
      db_.db_ << "rollback";
    }
  }

  IndexDbImpl::Tx::~Tx() {
    rollback();
  }

  IndexDbImpl::IndexDbImpl(const std::string &db_filename)
      : db_(db_filename, kLogName) {}

  IndexDbImpl::Tx IndexDbImpl::beginTx() {
    return Tx(*this);
  }

  outcome::result<void> IndexDbImpl::initDb() {
    static const char *schema[] = {
        R"(CREATE TABLE IF NOT EXISTS tipsets (
            hash BLOB PRIMARY KEY,
            branch INTEGER NOT NULL,
            height INTEGER NOT NULL,
            parent_hash BLOB NOT NULL,
            parent_branch INTEGER NOT NULL,
            blocks BLOB NOT NULL
        )",

        R"(CREATE UNIQUE INDEX IF NOT EXISTS tipsets_b_h ON tipsets
            (branch, height)
        )",
    };

    try {
      auto tx = beginTx();

      for (auto sql : schema) {
        db_ << sql;
      }

      get_tipset_info_ = db_.createStatement(
          R"(SELECT hash,branch,height,parent_hash,parent_branch FROM tipsets
          WHERE hash=?
          )");

      get_tipset_blocks_ =
          db_.createStatement(R"(SELECT blocks FROM tipsets WHERE hash=?)");

      insert_tipset_ =
          db_.createStatement(R"(INSERT INTO tipsets VALUES(?,?,?,?,?,?))");

      tx.commit();
    } catch (const sqlite::sqlite_exception &e) {
      log()->error("cannot init index db ({}, {})", e.what(), e.get_sql());
      return Error::INDEXDB_CANNOT_CREATE;
    }

    return outcome::success();
  }

  outcome::result<void> IndexDbImpl::loadGraph() {
    std::map<BranchId, BranchInfo> branches;

    try {
      db_ << "SELECT branch,MIN(height),hash,parent_branch "
             "FROM tipsets GROUP BY branch"
          >> [&branches](BranchId branch,
                         Height height,
                         Blob hash,
                         BranchId parent_branch) {
              auto &info = branches[branch];
              info.id = branch;
              info.bottom = std::move(hash);
              info.bottom_height = height;
              info.parent = parent_branch;
            };

      if (branches.empty()) {
        // new db here
        return outcome::success();
      }

      bool error = false;

      db_ << "SELECT branch,MAX(height),hash "
             "FROM tipsets GROUP BY branch"
          >> [&branches, &error](BranchId branch, Height height, Blob hash) {
              if (error) {
                return;
              }
              auto it = branches.find(branch);
              if (it == branches.end()) {
                error = true;
                return;
              }
              auto &info = it->second;
              info.top = std::move(hash);
              info.top_height = height;
            };

      if (error) {
        log()->error("cannot load graph data integrity error");
        return Error::INDEXDB_EXECUTE_ERROR;
      }

    } catch (const sqlite::sqlite_exception &e) {
      log()->error("cannot load graph ({}, {})", e.what(), e.get_sql());
      return Error::INDEXDB_EXECUTE_ERROR;
    } catch (...) {
      log()->error("cannot load graph: unknown error");
      return Error::INDEXDB_EXECUTE_ERROR;
    }

    OUTCOME_TRY(graph_.load(std::move(branches)));

    branch_id_counter_ = graph_.getLastBranchId();

    return outcome::success();
  }

  outcome::result<std::shared_ptr<IndexDb>> createIndexDb(
      const std::string &db_filename) {
    try {
      auto db = std::make_shared<IndexDbImpl>(db_filename);
      OUTCOME_TRY(db->initDb());
      OUTCOME_TRY(db->loadGraph());
      return db;

    } catch (const std::exception &e) {
      log()->error("cannot create index db ({}): {}", db_filename, e.what());
    } catch (...) {
      log()->error("cannot create index db ({}): exception", db_filename);
    }
    return Error::INDEXDB_CANNOT_CREATE;
  }

}  // namespace fc::storage::indexdb

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::indexdb, Error, e) {
  using E = fc::storage::indexdb::Error;
  switch (e) {
    case E::INDEXDB_CANNOT_CREATE:
      return "indexdb: cannot open db";
    case E::INDEXDB_INVALID_ARGUMENT:
      return "indexdb: invalid argument";
    case E::INDEXDB_EXECUTE_ERROR:
      return "indexdb: query execute error";
    case E::INDEXDB_DECODE_ERROR:
      return "indexdb: decode error";
    default:
      break;
  }
  return "indexdb: unknown error";
}
