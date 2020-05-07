/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "indexdb_impl.hpp"

namespace fc::storage::indexdb {

  namespace {
    auto constexpr kLogName = "indexdb";

    auto log() {
      static common::Logger logger = common::createLogger(kLogName);
      return logger.get();
    }
  }  // namespace

  Tx::Tx(IndexDb &db) : db_(db) {}

  void Tx::commit() {
    db_.commitTx();
    done_ = true;
  }

  void Tx::rollback() {
    db_.rollbackTx();
    done_ = true;
  }

  Tx::~Tx() {
    if (!done_) db_.rollbackTx();
  }

  IndexDbImpl::IndexDbImpl(const std::string &db_filename)
      : db_(db_filename, kLogName) {
    init();
  }

  Tx IndexDbImpl::beginTx() {
    db_ << "begin";
    return Tx(*this);
  }

  void IndexDbImpl::commitTx() {
    db_ << "commit";
  }

  void IndexDbImpl::rollbackTx() {
    db_ << "rollback";
  }

  outcome::result<void> IndexDbImpl::getParents(
      const Blob &id, const std::function<void(const Blob &)> &cb) {
    bool res = db_.execQuery(get_parents_, cb, id);
    if (!res) {
      return Error::INDEXDB_EXECUTE_ERROR;
    }
    return outcome::success();
  }

  outcome::result<void> IndexDbImpl::setParent(const Blob &parent,
                                               const Blob &child) {
    int rows_affected = db_.execCommand(set_parent_, parent, child);
    if (rows_affected != 1) {
      return Error::INDEXDB_EXECUTE_ERROR;
    }
    return outcome::success();
  }

  void IndexDbImpl::init() {
    static const char *schema[] = {
        R"(CREATE TABLE IF NOT EXISTS cids (
            cid BLOB PRIMARY KEY,
            type INTEGER NOT NULL,
            state INTEGER NOT NULL,
            height INTEGER NOT NULL))",
        R"(CREATE TABLE IF NOT EXISTS links (
            left BLOB NOT NULL,
            right BLOB NOT NULL,
            UNIQUE (left, right)))",
        R"(CREATE INDEX IF NOT EXISTS links_l ON links
            (left))",
        R"(CREATE INDEX IF NOT EXISTS links_r ON links
            (right))",
        R"(CREATE TABLE IF NOT EXISTS tipsets (
            hash BLOB PRIMARY KEY,
            state INTEGER NOT NULL,
            height INTEGER NOT NULL))",
        R"(CREATE TABLE IF NOT EXISTS blocks (
            tipset BLOB REFERENCES tipsets(hash),
            cid BLOB REFERENCES cids(cid),
            seq INTEGER NOT NULL,
            UNIQUE (tipset, cid, seq)))",
    };

    try {
      db_ << "PRAGMA foreign_keys = ON";
      auto tx = beginTx();

      for (auto sql : schema) {
        db_ << sql;
      }

      get_parents_ =
          db_.createStatement("SELECT left FROM links WHERE right=?");

      set_parent_ = db_.createStatement("INSERT INTO links VALUES(?,?)");

      get_successors_ =
          db_.createStatement("SELECT right FROM links WHERE left=?");

      tx.commit();
    } catch (sqlite::sqlite_exception &e) {
      throw std::runtime_error(e.get_sql());
    }
  }

  outcome::result<std::shared_ptr<IndexDb>> createIndexDb(
      const std::string &db_filename) {
    try {
      return std::make_shared<IndexDbImpl>(db_filename);
    } catch (const std::exception &e) {
      log()->error("cannot create index db ({}): {}", db_filename, e.what());
    } catch (...) {
      log()->error("cannot create index db ({}): exception", db_filename);
    }
    return Error::INDEXDB_CANNOT_CREATE;
  }

  outcome::result<void> setParent(IndexDb &db,
                                  const CID &parent,
                                  const CID &child) {
    OUTCOME_TRY(parent_bytes, parent.toBytes());
    OUTCOME_TRY(child_bytes, child.toBytes());
    return db.setParent(parent_bytes, child_bytes);
  }

  outcome::result<std::vector<CID>> getParents(IndexDb &db, const CID &cid) {
    std::vector<CID> cids;
    bool decode_error = false;
    OUTCOME_TRY(parent_bytes, cid.toBytes());
    OUTCOME_TRY(db.getParents(parent_bytes, [&](const IndexDb::Blob& b) {
      if (!decode_error) {
        gsl::span<const uint8_t> sp(b);
        auto res = CID::read(sp, false);
        if (!res) {
          log()->error("CID decode error: {}", res.error().message());
          decode_error = true;
        } else {
          cids.push_back(std::move(res.value()));
        }
      }
    }));
    if (decode_error) {
      return Error::INDEXDB_DECODE_ERROR;
    }
    return cids;
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
