/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <fstream>
#include <mutex>
#include <shared_mutex>

#include "cbor_blake/ipld.hpp"
#include "common/outcome2.hpp"
#include "primitives/cid/cid.hpp"
#include "storage/car/cids_index/cids_index.hpp"
#include "storage/ipfs/datastore.hpp"

namespace fc::storage::ipld {
  using cids_index::Index;
  using cids_index::Row;

  struct CidsIpld : CbIpld,
                    public Ipld,
                    public std::enable_shared_from_this<CidsIpld> {
    using CbIpld::get, CbIpld::put;

    outcome::result<bool> contains(const CID &cid) const override;
    outcome::result<void> set(const CID &cid, Bytes value) override;
    outcome::result<void> set(const CID &cid, BytesIn value) override;
    outcome::result<Bytes> get(const CID &cid) const override;

    bool get(const CbCid &key, Bytes *value) const override;
    void put(const CbCid &key, BytesIn value) override;

    void asyncFlush();

    inline boost::optional<Row> findWritten(const CbCid &key) const;
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
    std::mutex flush_mutex;
    size_t flush_on{};
    std::shared_ptr<boost::asio::io_context> io;
    std::string index_path;
    std::string car_path;
    boost::optional<size_t> max_memory;
  };
}  // namespace fc::storage::ipld
