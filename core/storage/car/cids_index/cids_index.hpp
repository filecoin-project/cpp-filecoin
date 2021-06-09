/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/endian/buffers.hpp>
#include <fstream>
#include <libp2p/common/metrics/instance_count.hpp>
#include <mutex>

#include "common/blob.hpp"
#include "common/enum.hpp"
#include "storage/ipfs/datastore.hpp"

namespace boost {
  namespace asio {
    class io_context;
  }  // namespace asio
}  // namespace boost

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

  struct Progress;

  outcome::result<size_t> checkIndex(std::ifstream &file);

  std::pair<bool, size_t> readCarItem(std::istream &car_file,
                                      const Row &row,
                                      uint64_t *end);

  struct RowsInfo {
    bool valid{true};
    bool sorted{true};
    size_t count{};
    Row max_offset;
    Key max_key;

    RowsInfo &feed(const Row &row);
  };

  struct MergeRange {
    std::istream *file{};
    std::vector<Row> rows;
    size_t current{(size_t)-1}, begin{}, end{};

    inline auto &front() const {
      assert(current < rows.size());
      return rows[current];
    }

    bool empty() const;
    bool read();
    void pop();
  };
  inline auto operator>(const MergeRange &l, const MergeRange &r) {
    return r.front() < l.front();
  }

  outcome::result<void> merge(std::ostream &out,
                              std::vector<MergeRange> &&ranges);

  outcome::result<size_t> readCar(std::istream &car_file,
                                  uint64_t car_min,
                                  uint64_t car_max,
                                  boost::optional<size_t> max_memory,
                                  IpldPtr ipld,
                                  Progress *progress,
                                  std::fstream &rows_file,
                                  std::vector<MergeRange> &ranges);

  struct Index {
    RowsInfo info;

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
        std::ifstream &file, size_t count);

    LIBP2P_METRICS_INSTANCE_COUNT(fc::storage::cids_index::MemoryIndex);
  };

  struct SparseRange {
    size_t total{}, buckets{}, bucket_size{}, bucket_split{};

    SparseRange() = default;
    SparseRange(size_t total, size_t max_buckets);
    size_t fromSparse(size_t bucket) const;
  };

  struct SparseIndex : Index {
    mutable std::mutex mutex;
    mutable std::ifstream index_file;
    SparseRange sparse_range;
    std::vector<Key> sparse_keys;

    outcome::result<boost::optional<Row>> find(const Key &key) const override;
    size_t size() const override;

    static outcome::result<std::shared_ptr<SparseIndex>> load(
        std::ifstream &&file, size_t count, size_t max_keys);

    LIBP2P_METRICS_INSTANCE_COUNT(fc::storage::cids_index::SparseIndex);
  };

  outcome::result<std::shared_ptr<Index>> load(
      const std::string &index_path, boost::optional<size_t> max_memory);
}  // namespace fc::storage::cids_index
