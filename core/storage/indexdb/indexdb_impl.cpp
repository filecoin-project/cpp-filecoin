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
    // TODO LRU cache

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

  /*
  outcome::result<void> IndexDbImpl::applyToTop(const Tipset &tipset,
                                                // TODO parent hash
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

   */

  //
  //  outcome::result<void> IndexDbImpl::eraseChain(const TipsetHash &from) {
  //    // TODO nyi
  //    return Error::INDEXDB_INVALID_ARGUMENT;
  //  }

  outcome::result<IndexDb::ApplyResult> IndexDbImpl::applyTipset(
      const Tipset &tipset,
      bool parent_must_exist,
      boost::optional<const TipsetHash &> parent,
      boost::optional<const TipsetHash &> successor) {
    // Variants of tipset indexing:
    // 1) parent_must_exist && successor: linking branches after doing (2) and
    // (3) - this typically ends synchronisation process;
    //
    // 2) !parent_must_exist && successor: applying the tipset to the
    // bottom of successor's branch;
    //
    // 3) parent_must_exist && !successor: applying the tipset on top of parent:
    //   3a) if parent is a tip (i.e. head) then continuing parent's branch;
    //   3b) if parent is a top of a branch then making a fork;
    //   3c) if parent is in the middle of its branch then making a fork and
    //   splitting parent's branch;
    //
    // 4) !parent_must_exist && !successor: inserting genesis tipset into empty
    // db.

    boost::optional<TipsetInfo> parent_info;
    boost::optional<TipsetInfo> successor_info;
    BranchId parent_branch = kNoBranch;
    bool parent_is_top = false;
    bool parent_is_head = false;
    BranchId successor_branch = kNoBranch;
    bool new_branch_created = false;

    if (parent_must_exist) {
      // then we are linking to existing branch

      if (!parent) {
        return Error::INDEXDB_INVALID_ARGUMENT;
      }

      OUTCOME_TRYA(parent_info, getTipsetInfo(parent.value()));

      if (parent_info->height >= tipset.height()) {
        return Error::LINK_HEIGHT_MISMATCH;
      }

      parent_branch = parent_info->branch;

      OUTCOME_TRY(b, graph_.getBranch(parent_branch));
      const auto &info = b.get();
      if (info.top == parent.value()) {
        parent_is_top = true;
      }
      if (parent_is_top && info.forks.empty()) {
        parent_is_head = true;
      }
    }

    if (successor) {
      // applying to the bottom of successor's branch, it must exist

      OUTCOME_TRYA(successor_info, getTipsetInfo(successor.value()));

      if (successor_info->height <= tipset.height()) {
        return Error::LINK_HEIGHT_MISMATCH;
      }

      if (successor_info->parent_hash != tipset.key.hash()) {
        return Error::UNEXPECTED_TIPSET_PARENT;
      }

      if (successor_info->parent_branch != kNoBranch) {
        return Error::BRANCH_IS_NOT_A_ROOT;
      }

      successor_branch = successor_info->branch;

      OUTCOME_TRY(b, graph_.getBranch(successor_branch));
      const auto &info = b.get();
      if (info.bottom != successor.value()) {
        return Error::BRANCH_IS_NOT_A_ROOT;
      }
    }

    BranchId branch_assigned = kNoBranch;
    if (successor) {
      branch_assigned = successor_branch;
    } else if (parent_is_head) {
      branch_assigned = parent_branch;
    }

    if (!parent_must_exist && !successor) {
      // inserting genesis branch
      if (!graph_.empty()) {
        return Error::INDEXDB_MUST_BE_EMPTY;
      }
      branch_assigned = kGenesisBranch;
      new_branch_created = true;
    }

    if (branch_assigned == kNoBranch) {
      branch_assigned = ++branch_id_counter_;
      new_branch_created = true;
    }

    OUTCOME_TRY(buffer, codec::cbor::encode(tipset.key.cids()));

    auto tx = beginTx();

    int rows = 0;

    if (parent) {
      rows = db_.execCommand(insert_tipset_,
                             tipset.key.hash(),
                             branch_assigned,
                             tipset.height(),
                             parent.value(),
                             parent_branch,
                             buffer.toVector());
    } else {
      assert(branch_assigned == kGenesisBranch);
      assert(tipset.height() == 0);
      rows = db_.execCommand(insert_tipset_,
                             tipset.key.hash(),
                             kGenesisBranch,
                             0,
                             "",
                             kNoBranch,
                             buffer.toVector());
    }

    if (rows != 1) {
      return Error::INDEXDB_EXECUTE_ERROR;
    }

    // split parent branch into 2 branches
    BranchId branch_splitted = kNoBranch;

    if (parent_is_head) {
      //      rename_branch_ = db_.createStatement(
      //          R"(UPDATE tipsets SET branch=? WHERE branch=? AND
      //          height>=?)");
      //
      //      rename_parent_branch_ = db_.createStatement(
      //          R"(UPDATE tipsets SET parent_branch=? WHERE branch=?)");
      //
      if (branch_assigned != parent_branch) {
        // renaming successor branch -> parent branch
        rows =
            db_.execCommand(rename_branch_, parent_branch, branch_assigned, 0);
        if (rows <= 0) {
          return Error::INDEXDB_EXECUTE_ERROR;
        }
        rows = db_.execCommand(
            rename_parent_branch_, parent_branch, branch_assigned);
        if (rows < 0) {
          return Error::INDEXDB_EXECUTE_ERROR;
        }

        OUTCOME_TRY(graph_.mergeBranches(parent_branch, branch_assigned));
        branch_assigned = parent_branch;

      } else {
        // making tipset a new head
        assert(!successor);

        OUTCOME_TRY(graph_.updateTop(
            parent_branch, tipset.key.hash(), tipset.height()));
      }
    } else {
      if (new_branch_created) {
        // new branch created which contains this single tipset
        OUTCOME_TRY(graph_.newBranch(
            branch_assigned, tipset.key.hash(), tipset.height()));
      } else if (branch_assigned == successor_branch) {
        // tipset was appended to bottom of existing branch
        OUTCOME_TRY(graph_.updateBottom(
            branch_assigned, tipset.key.hash(), tipset.height()));
      }

      if (parent_must_exist) {
        if (parent_is_top) {
          // make fork from parent branch which is not a head
          

        } else {
          // making fork in the middle of existing branch, thus producing a new
          // branch
          branch_splitted = ++branch_id_counter_;

          // find existing successor of parent and make sure it's on the same
          // branch


        }
      }
    }

    if (successor_branch != kNoBranch && !parent_must_exist) {
    }

    if (parent_is_top && !parent_is_head) {
      branches_merged = true;
    }

    if (parent_must_exist && !parent_is_top) {
      branches_merged = true;
    }

    tx.commit();

    return ApplyResult{.on_top_of_branch = parent_is_top,
                       .on_bottom_of_branch = successor.has_value(),
                       .branches_merged = (branch_assigned == parent_branch),
                       .branches_splitted = (branch_splitted != kNoBranch),
                       .this_branch = branch_assigned,
                       .parent_branch = parent_branch,
                       .splitted_branch = branch_splitted};
  }

  outcome::result<void> IndexDbImpl::writeGenesis(const Tipset &tipset) {
    OUTCOME_TRY(applyTipset(tipset, false, boost::none, boost::none));
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
            parent_hash BLOB NOT NULL
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
          R"(SELECT hash,branch,height,parent_hash FROM tipsets
          WHERE hash=?
          )");

      insert_tipset_ =
          db_.createStatement(R"(INSERT INTO tipsets VALUES(?,?,?,?,?,?))");

      rename_branch_ = db_.createStatement(
          R"(UPDATE tipsets SET branch=? WHERE branch=? AND height>?)");


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

    if (!graph_.empty()) {
      branch_id_counter_ = graph_.getLastBranchId();
    }

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
