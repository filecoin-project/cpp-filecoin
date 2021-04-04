/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/endian/buffers.hpp>
#include <fstream>
#include <mutex>

#include "common/blob.hpp"
#include "common/enum.hpp"
#include "storage/ipfs/datastore.hpp"

namespace fc::storage::cids_index {
  using Key = common::Hash256;

  constexpr std::array<uint8_t, 6> kCborBlakePrefix{
      0x01, 0x71, 0xA0, 0xE4, 0x02, 0x20};

  constexpr size_t ceilDiv(size_t l, size_t r) {
    return (l + r - 1) / r;
  }

  /** max_size64 = ceil(size / 64) */
  constexpr size_t maxSize64(size_t size) {
    return ceilDiv(size, 64);
  }
  /** size <= max_size64 * 64 */
  constexpr size_t maxSize(size_t max_size64) {
    return max_size64 * 64;
  }

  enum class Meta : uint8_t {
    kHeaderV0 = 1,
    kTrailerV0 = 2,
  };

  struct Row {
    /** fixed-size key */
    Key key;
    /** 40bit offset, up to 1TB */
    boost::endian::big_uint40_buf_t offset;
    /** 24bit(+6bit) size, up to 1GB */
    /** size64=0 means this row is meta */
    boost::endian::big_uint24_buf_t max_size64;

    inline bool isMeta() const {
      return max_size64.value() == 0;
    }
  };
  static_assert(sizeof(Row) == 40);
  inline bool operator==(const Row &l, const Row &r) {
    return memcmp(&l, &r, sizeof(Row)) == 0;
  }
  inline bool operator!=(const Row &l, const Row &r) {
    return memcmp(&l, &r, sizeof(Row)) != 0;
  }
  inline bool operator<(const Row &l, const Row &r) {
    return memcmp(&l, &r, sizeof(Row)) < 0;
  }
  inline bool operator<(const Row &l, const Key &r) {
    return memcmp(&l.key, &r, sizeof(Key)) < 0;
  }

  static inline const Row kHeaderV0{
      {}, decltype(Row::offset){common::to_int(Meta::kHeaderV0)}, {}};
  static inline const Row kTrailerV0{
      {}, decltype(Row::offset){common::to_int(Meta::kTrailerV0)}, {}};

  struct RowsInfo {
    bool valid{true};
    bool sorted{true};
    size_t count{};
    uint64_t max_offset{};
    Key max_key;

    RowsInfo &feed(const Row &row);
  };

  struct Index {
    virtual ~Index() = default;
    virtual outcome::result<boost::optional<Row>> find(
        const Key &key) const = 0;
    virtual size_t size() const = 0;
  };

  struct MemoryIndex : Index {
    std::vector<Row> rows;

    outcome::result<boost::optional<Row>> find(const Key &key) const override;
    size_t size() const override;

    static outcome::result<std::shared_ptr<MemoryIndex>> load(
        std::ifstream &file);
  };

  // TODO(turuslan): sparse index when car is too big
  outcome::result<std::shared_ptr<Index>> load(const std::string &index_path);

  struct Progress;
  // TODO(turuslan): tmp file and sparse index when car is too big
  outcome::result<std::shared_ptr<Index>> create(const std::string &car_path,
                                                 const std::string &index_path,
                                                 IpldPtr ipld,
                                                 Progress *progress);

  struct CidsIpld : public Ipld, public std::enable_shared_from_this<CidsIpld> {
    CidsIpld(const std::string &car_path,
             std::shared_ptr<Index> index,
             IpldPtr ipld);
    outcome::result<bool> contains(const CID &cid) const override;
    outcome::result<void> set(const CID &cid, Buffer value) override;
    outcome::result<Buffer> get(const CID &cid) const override;
    outcome::result<void> remove(const CID &cid) override {
      throw "deprecated";
    }
    IpldPtr shared() override {
      return shared_from_this();
    }

    mutable std::mutex mutex;
    mutable std::ifstream car_file;
    std::shared_ptr<Index> index;
    IpldPtr ipld;
  };
}  // namespace fc::storage::cids_index
