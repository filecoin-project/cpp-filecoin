/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <fstream>
#include <mutex>
#include <shared_mutex>

#include "common/outcome2.hpp"
#include "primitives/cid/cid.hpp"
#include "storage/car/cids_index/cids_index.hpp"
#include "storage/ipfs/datastore.hpp"
#include "storage/ipld/light_ipld.hpp"

namespace fc::storage::ipld {
  using cids_index::Index;
  using cids_index::Key;
  using cids_index::Row;

  class CidsIpld : LightIpld,
                   public Ipld,
                   public std::enable_shared_from_this<CidsIpld> {
   public:
    using LightIpld::get;

    outcome::result<bool> contains(const CID &cid) const override;
    outcome::result<void> set(const CID &cid, Buffer value) override;
    outcome::result<Buffer> get(const CID &cid) const override;
    outcome::result<void> remove(const CID &cid) override {
      throw "deprecated";
    }
    IpldPtr shared() override {
      return shared_from_this();
    }

    bool get(const Hash256 &key, Buffer *value) const override;
    void put(const Hash256 &key, BytesIn value) override;

    void asyncFlush();

    inline boost::optional<Row> findWritten(const Key &key) const;
    Outcome<void> doFlush();

    mutable std::mutex car_mutex;
    mutable std::ifstream car_file;
    mutable std::shared_mutex index_mutex;
    std::shared_ptr<Index> index;
    IpldPtr ipld;
    std::ofstream writable;
    mutable std::shared_mutex written_mutex;
    std::set<Row> written;
    uint64_t car_offset{};
    std::atomic_flag flushing;
    size_t flush_on{};
    std::shared_ptr<boost::asio::io_context> io;
    std::string index_path;
    boost::optional<size_t> max_memory;
  };

  struct Ipld2Ipld : public Ipld,
                     public std::enable_shared_from_this<Ipld2Ipld> {
    LightIpldPtr ipld;

    outcome::result<bool> contains(const CID &cid) const override;
    outcome::result<void> set(const CID &cid, Buffer value) override;
    outcome::result<Buffer> get(const CID &cid) const override;
    outcome::result<void> remove(const CID &cid) override {
      throw "deprecated";
    }
    IpldPtr shared() override {
      return shared_from_this();
    }
  };
}  // namespace fc::storage::ipld
