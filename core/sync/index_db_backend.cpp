/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/multi/content_identifier_codec.hpp>

#include "index_db_backend.hpp"

namespace fc::sync {

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("indexdb");
      return logger.get();
    }

    constexpr size_t kBytesInHash = 32;

    outcome::result<std::vector<uint8_t>> encodeCids(
        const std::vector<CID> &cids) {
      std::vector<uint8_t> buffer;
      size_t sz = cids.size();
      if (sz != 0) {
        buffer.resize(sz * kBytesInHash);
        size_t offset = 0;
        for (const auto &cid : cids) {
          auto hash_raw = cid.content_address.getHash();
          if (hash_raw.size() != kBytesInHash) {
            return Error::SYNC_DATA_INTEGRITY_ERROR;
          }
          memcpy(buffer.data() + offset, hash_raw.data(), kBytesInHash);
          offset += kBytesInHash;
        }
      }
      return buffer;
    }

    outcome::result<std::vector<CID>> decodeCids(
        gsl::span<const uint8_t> bytes) {
      size_t sz = bytes.size();
      if (sz % kBytesInHash != 0) {
        return Error::SYNC_DATA_INTEGRITY_ERROR;
      }
      std::vector<CID> cids;
      if (sz != 0) {
        cids.reserve(sz / kBytesInHash);

        for (size_t offset = 0; offset < sz; offset += kBytesInHash) {
          gsl::span<const uint8_t> hash_raw =
              bytes.subspan(offset, kBytesInHash);
          OUTCOME_TRY(hash,
                      libp2p::multi::Multihash::create(
                          libp2p::multi::HashType::blake2b_256, hash_raw));
          cids.emplace_back(
              CID::Version::V1, CID::Multicodec::DAG_CBOR, std::move(hash));
        }
      }
      return cids;
    }

  }  // namespace

  outcome::result<std::shared_ptr<IndexDbBackend>> IndexDbBackend::create(
      const std::string &db_filename) {
    try {
      return std::make_shared<IndexDbBackend>(db_filename);
    } catch (const std::exception &e) {
      log()->error("cannot create: {}", e.what());
    }
    return Error::INDEXDB_CANNOT_CREATE;
  }

  outcome::result<void> IndexDbBackend::store(
      const TipsetInfo &info,
      const boost::optional<RenameBranch> &branch_rename) {
    int rows = 0;

    OUTCOME_TRY(cids, encodeCids(info.key.cids()));

    if (!info.parent_hash.empty()) {
      rows = db_.execCommand(insert_tipset_,
                             info.key.hash(),
                             info.branch,
                             info.height,
                             info.parent_hash,
                             cids);
    } else {
      assert(info.branch == kGenesisBranch);
      assert(info.height == 0);
      rows = db_.execCommand(
          insert_tipset_, info.key.hash(), kGenesisBranch, 0, "", cids);
    }

    if (rows != 1) {
      return Error::INDEXDB_EXECUTE_ERROR;
    }

    if (branch_rename) {
      rows = db_.execCommand(rename_branch_,
                             branch_rename->new_id,
                             branch_rename->old_id,
                             branch_rename->above_height);
    }

    if (rows < 0) {
      return Error::INDEXDB_EXECUTE_ERROR;
    }

    return outcome::success();
  }

  outcome::result<IndexDbBackend::TipsetIdx> IndexDbBackend::get(
      const TipsetHash &hash) {
    TipsetIdx idx;
    auto cb = [&idx](TipsetHash hash,
                     BranchId branch,
                     Height height,
                     TipsetHash parent_hash,
                     Blob cids) {
      idx.hash = std::move(hash);
      idx.branch = branch;
      idx.height = height;
      idx.parent_hash = std::move(parent_hash);
      idx.cids = std::move(cids);
    };
    bool res = db_.execQuery(get_by_hash_, cb, hash);
    if (!res) {
      return Error::INDEXDB_EXECUTE_ERROR;
    }
    if (idx.hash.empty()) {
      return Error::INDEXDB_TIPSET_NOT_FOUND;
    }
    return idx;
  }

  outcome::result<IndexDbBackend::TipsetIdx> IndexDbBackend::get(
      BranchId branch, Height height) {
    TipsetIdx idx;
    auto cb = [&idx](TipsetIdx raw) { idx = std::move(raw); };
    OUTCOME_TRY(walk(branch, height, 1, cb));
    if (idx.hash.empty()) {
      return Error::INDEXDB_TIPSET_NOT_FOUND;
    }
    return idx;
  }

  outcome::result<std::shared_ptr<TipsetInfo>> IndexDbBackend::decode(
      TipsetIdx raw) {
    OUTCOME_TRY(cids, decodeCids(raw.cids));
    return std::make_shared<TipsetInfo>(
        TipsetInfo{TipsetKey::create(std::move(cids), std::move(raw.hash)),
                   raw.branch,
                   raw.height,
                   std::move(raw.parent_hash)});
  }

  outcome::result<void> IndexDbBackend::walk(
      BranchId branch,
      Height height,
      uint64_t limit,
      const std::function<void(TipsetIdx raw)> &cb) {
    auto raw_cb = [&cb](TipsetHash hash,
                        BranchId branch,
                        Height height,
                        TipsetHash parent_hash,
                        Blob cids) {
      cb(TipsetIdx{std::move(hash),
                   branch,
                   height,
                   std::move(parent_hash),
                   std::move(cids)});
    };

    bool res = db_.execQuery(get_by_position_, raw_cb, branch, height, limit);
    if (!res) {
      return Error::INDEXDB_EXECUTE_ERROR;
    }
    return outcome::success();
  }

  outcome::result<std::map<BranchId, std::shared_ptr<BranchInfo>>>
  IndexDbBackend::initDb() {
    static const char *schema[] = {
        R"(CREATE TABLE IF NOT EXISTS tipsets (
            hash BLOB PRIMARY KEY,
            branch INTEGER NOT NULL,
            height INTEGER NOT NULL,
            parent_hash BLOB NOT NULL,
            cids BLOB NOT NULL)
        )",

        R"(CREATE UNIQUE INDEX IF NOT EXISTS tipsets_b_h ON tipsets
            (branch, height)
        )",
    };

    std::map<BranchId, std::shared_ptr<BranchInfo>> branches;

    try {
      auto tx = beginTx();

      for (auto sql : schema) {
        db_ << sql;
      }

      get_by_hash_ = db_.createStatement(
          R"(SELECT hash,branch,height,parent_hash,cids FROM tipsets
          WHERE hash=?
          )");

      get_by_position_ = db_.createStatement(
          R"(SELECT hash,branch,height,parent_hash,cids FROM tipsets
          WHERE branch=? AND height>=? LIMIT ?
          )");

      insert_tipset_ =
          db_.createStatement(R"(INSERT INTO tipsets VALUES(?,?,?,?,?))");

      rename_branch_ = db_.createStatement(
          R"(UPDATE tipsets SET branch=? WHERE branch=? AND height>?)");

      tx.commit();

      db_ << "SELECT branch,MIN(height),hash,parent_hash "
             "FROM tipsets GROUP BY branch"
          >> [&branches](BranchId branch,
                         Height height,
                         TipsetHash hash,
                         TipsetHash parent_hash) {
              auto &info = branches[branch];
              info = std::make_shared<BranchInfo>();
              info->id = branch;
              info->bottom = std::move(hash);
              info->bottom_height = height;
              info->parent_hash = std::move(parent_hash);
            };

      if (branches.empty()) {
        // new db here
        return branches;
      }

      bool error = false;

      db_ << "SELECT branch,MAX(height),hash "
             "FROM tipsets GROUP BY branch"
          >>
          [&branches, &error](BranchId branch, Height height, TipsetHash hash) {
            if (error) {
              return;
            }
            auto it = branches.find(branch);
            if (it == branches.end()) {
              error = true;
              return;
            }
            auto &info = it->second;
            info->top = std::move(hash);
            info->top_height = height;
          };

      if (error) {
        log()->error("cannot load graph: data integrity error");
        return Error::INDEXDB_EXECUTE_ERROR;
      }

      std::map<TipsetHash, BranchId> tops;
      for (const auto &[id, info] : branches) {
        tops[info->top] = id;
      }

      for (auto &[_, info] : branches) {
        auto it = tops.find(info->parent_hash);
        if (it != tops.end()) {
          info->parent = it->second;
        }
      }

    } catch (const sqlite::sqlite_exception &e) {
      log()->error("cannot load graph ({}, {})", e.what(), e.get_sql());
      return Error::INDEXDB_EXECUTE_ERROR;
    } catch (...) {
      log()->error("cannot load graph: unknown error");
      return Error::INDEXDB_CANNOT_CREATE;
    }

    return branches;
  }

  IndexDbBackend::Tx::Tx(IndexDbBackend &db) : db_(db) {
    db_.db_ << "begin";
  }

  void IndexDbBackend::Tx::commit() {
    if (!done_) {
      done_ = true;
      db_.db_ << "commit";
    }
  }

  void IndexDbBackend::Tx::rollback() {
    if (!done_) {
      done_ = true;
      db_.db_ << "rollback";
    }
  }

  IndexDbBackend::Tx::~Tx() {
    rollback();
  }

  IndexDbBackend::IndexDbBackend(const std::string &db_filename)
      : db_(db_filename, "indexdb") {}

  IndexDbBackend::Tx IndexDbBackend::beginTx() {
    return Tx(*this);
  }

}  // namespace fc::sync
