/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/car/cids_index/cids_index.hpp"

#include <boost/filesystem/operations.hpp>

#include "codec/uvarint.hpp"
#include "common/error_text.hpp"
#include "common/file.hpp"
#include "common/from_span.hpp"
#include "storage/car/cids_index/progress.hpp"
#include "storage/ipfs/ipfs_datastore_error.hpp"

namespace fc::storage::cids_index {
  inline bool read(std::istream &is, Row &row) {
    return common::read(is, gsl::make_span(&row, 1));
  }

  outcome::result<size_t> checkIndex(std::ifstream &file) {
    file.seekg(0, std::ios::end);
    auto _size{file.tellg()};
    if (_size < 0) {
      return ERROR_TEXT("checkIndex: get file size failed");
    }
    auto size{(size_t)_size};
    if (size % sizeof(Row) != 0) {
      return ERROR_TEXT("checkIndex: invalid file size");
    }
    file.seekg(0);
    Row header;
    if (!read(file, header)) {
      return ERROR_TEXT("checkIndex: read header failed");
    }
    if (header != kHeaderV0) {
      return ERROR_TEXT("checkIndex: invalid header");
    }
    file.seekg(-sizeof(Row), std::ios::end);
    Row trailer;
    if (!read(file, trailer)) {
      return ERROR_TEXT("checkIndex: read trailer failed");
    }
    if (trailer != kTrailerV0) {
      return ERROR_TEXT("checkIndex: invalid trailer");
    }
    file.seekg(sizeof(Row));
    return size / sizeof(Row) - 2;
  }

  std::pair<bool, size_t> readCarItem(std::ifstream &car_file,
                                      const Row &row,
                                      uint64_t *end) {
    car_file.seekg(row.offset.value());
    auto prefix{kCborBlakePrefix};
    Key key;
    codec::uvarint::VarintDecoder varint;
    if (read(car_file, varint)) {
      if (common::read(car_file, prefix) && prefix == kCborBlakePrefix) {
        if (common::read(car_file, key) && key == row.key) {
          if (end) {
            *end = row.offset.value() + varint.length + varint.value;
          }
          return {true, varint.value - prefix.size() - key.size()};
        }
      }
    }
    return {false, 0};
  }

  RowsInfo &RowsInfo::feed(const Row &row) {
    valid = valid && !row.isMeta();
    if (valid) {
      sorted = count == 0 || max_key < row.key;
      valid = sorted;
      ++count;
      if (row.offset.value() > max_offset.offset.value()) {
        max_offset = row;
      }
      max_key = std::max(max_key, row.key);
    }
    return *this;
  }

  outcome::result<std::shared_ptr<Index>> create(const std::string &car_path,
                                                 const std::string &index_path,
                                                 IpldPtr ipld,
                                                 Progress *progress) {
    auto write_error{ERROR_TEXT("cids_index create: write error")};
    boost::system::error_code ec;
    auto init_car_size{boost::filesystem::file_size(car_path, ec)};
    if (ec) {
      return ec;
    }
    if (progress) {
      progress->car_size = init_car_size;
      progress->begin();
    }
    auto BOOST_OUTCOME_TRY_UNIQUE_NAME{gsl::finally([&] {
      if (progress) {
        progress->end();
      }
    })};
    if (init_car_size > (size_t{40} << 30)) {
      return ERROR_TEXT(
          "cids_index create: TODO: safe indexing for car over 40gb");
    }
    std::ifstream car_file{car_path, std::ios::binary};
    // estimated
    car_file.rdbuf()->pubsetbuf(nullptr, 64 << 10);

    size_t offset{};
    Buffer header;
    if (auto varint{codec::uvarint::readBytes(car_file, header)}) {
      offset += varint + header.size();
    } else {
      return ERROR_TEXT("cids_index create: read header failed");
    }

    std::vector<Row> rows;
    // estimated
    rows.reserve(init_car_size * 33 / 23520);

    Buffer item;
    while (offset < init_car_size) {
      auto varint{codec::uvarint::readBytes(car_file, item)};
      if (!varint) {
        break;
      }
      BytesIn input{item};
      auto size{varint + item.size()};
      auto stored{false};
      if (startsWith(item, kCborBlakePrefix)) {
        input = input.subspan(kCborBlakePrefix.size());
        OUTCOME_TRY(key, fromSpan<Key>(input, false));
        auto &row{rows.emplace_back()};
        row.key = key;
        row.offset = offset;
        row.max_size64 = maxSize64(size);
        stored = true;
      }
      if (!stored && ipld) {
        OUTCOME_TRY(cid, CID::read(input));
        OUTCOME_TRY(ipld->set(cid, Buffer{input}));
      }
      offset += size;

      if (progress) {
        progress->car_offset.value = offset;
        ++progress->items.value;
        progress->update();
      }
    }

    auto car_size{boost::filesystem::file_size(car_path, ec)};
    if (ec) {
      return ec;
    }
    if (car_size != init_car_size) {
      return ERROR_TEXT("cids_index create: car size changed");
    }
    if (offset != init_car_size) {
      return ERROR_TEXT("cids_index create: invalid car");
    }

    if (progress) {
      progress->sort();
    }
    std::sort(rows.begin(), rows.end());

    auto tmp_index_path{index_path + ".tmp"};
    std::ofstream index_file{tmp_index_path, std::ios::binary};
    if (!common::write(index_file, gsl::make_span(&kHeaderV0, 1))) {
      return write_error;
    }
    if (!common::write(index_file, gsl::make_span(rows))) {
      return write_error;
    }
    if (!common::write(index_file, gsl::make_span(&kTrailerV0, 1))) {
      return write_error;
    }
    index_file.close();
    boost::filesystem::rename(tmp_index_path, index_path, ec);
    if (ec) {
      return ec;
    }

    auto index{std::make_shared<MemoryIndex>()};
    index->rows = std::move(rows);
    return index;
  }

  outcome::result<boost::optional<Row>> MemoryIndex::find(
      const Key &key) const {
    auto it{std::lower_bound(rows.begin(), rows.end(), key)};
    if (it != rows.end() && it->key == key) {
      if (it->isMeta()) {
        return ERROR_TEXT("MemoryIndex.find: inconsistent");
      }
      return *it;
    }
    return boost::none;
  }

  size_t MemoryIndex::size() const {
    return rows.size();
  }

  outcome::result<std::shared_ptr<MemoryIndex>> MemoryIndex::load(
      std::ifstream &file, size_t count) {
    auto index{std::make_shared<MemoryIndex>()};
    index->rows.resize(count);
    if (!common::read(file, gsl::make_span(index->rows))) {
      return ERROR_TEXT("MemoryIndex::load: read rows failed");
    }
    for (auto &row : index->rows) {
      if (!index->info.feed(row).valid) {
        return ERROR_TEXT("MemoryIndex::load: invalid index");
      }
    }
    return index;
  }

  SparseRange::SparseRange(size_t total, size_t max_buckets) : total{total} {
    if (total == 1) {
      buckets = 1;
      bucket_size = 1;
    } else if (total != 0) {
      buckets = std::clamp<size_t>(max_buckets, 2, total);
      bucket_size = (total - 1) / (buckets - 1);
      bucket_split = (total - 1) % (buckets - 1);
    }
  }

  size_t SparseRange::fromSparse(size_t bucket) const {
    assert(bucket_size != 0);
    assert(buckets >= 1);
    assert(buckets <= total);
    assert(bucket < buckets);
    if (bucket == buckets - 1) {
      return total - 1;
    }
    return bucket * bucket_size + std::min(bucket, bucket_split);
  }

  outcome::result<boost::optional<Row>> SparseIndex::find(
      const Key &key) const {
    if (sparse_keys.empty() || key < sparse_keys[0]) {
      return boost::none;
    }
    auto it{std::lower_bound(sparse_keys.begin(), sparse_keys.end(), key)};
    if (it == sparse_keys.end()) {
      return boost::none;
    }
    auto i_sparse{it - sparse_keys.begin()};
    auto i_end{sparse_range.fromSparse(i_sparse)};
    auto i_begin{i_end};
    if (*it != key) {
      i_begin = sparse_range.fromSparse(i_sparse - 1) + 1;
      --i_end;
    }
    std::unique_lock lock{mutex};
    index_file.seekg((1 + i_begin) * sizeof(Row));
    for (auto i{i_begin}; i <= i_end; ++i) {
      Row row;
      if (!read(index_file, row)) {
        return ERROR_TEXT("SparseIndex.find: read error");
      }
      if (row.isMeta()) {
        return ERROR_TEXT("SparseIndex.find: inconsistent");
      }
      if (row.key == key) {
        return row;
      }
    }
    return boost::none;
  }

  size_t SparseIndex::size() const {
    return sparse_range.total;
  }

  outcome::result<std::shared_ptr<SparseIndex>> SparseIndex::load(
      std::ifstream &&file, size_t count, size_t max_keys) {
    auto index{std::make_shared<SparseIndex>()};
    index->sparse_range = {count, max_keys};
    index->sparse_keys.resize(index->sparse_range.buckets);
    for (size_t i_sparse{0}, i_row{0}; i_sparse < index->sparse_range.buckets;
         ++i_sparse) {
      auto i_next{index->sparse_range.fromSparse(i_sparse)};
      Row row;
      do {
        if (!read(file, row)) {
          return ERROR_TEXT("SparseIndex::load: read row failed");
        }
        if (!index->info.feed(row).valid) {
          return ERROR_TEXT("SparseIndex::load: invalid index");
        }
        ++i_row;
      } while (i_row <= i_next);
      index->sparse_keys[i_sparse] = row.key;
    }
    index->index_file = std::move(file);
    return index;
  }

  outcome::result<std::shared_ptr<Index>> load(const std::string &index_path,
                                               size_t max_memory) {
    std::ifstream index_file{index_path, std::ios::binary};
    // estimated
    index_file.rdbuf()->pubsetbuf(nullptr, 64 << 10);
    OUTCOME_TRY(count, checkIndex(index_file));
    if (count * sizeof(Row) > max_memory) {
      OUTCOME_TRY(index,
                  SparseIndex::load(
                      std::move(index_file), count, max_memory / sizeof(Key)));
      return std::move(index);
    }
    OUTCOME_TRY(index, MemoryIndex::load(index_file, count));
    return std::move(index);
  }

  CidsIpld::CidsIpld(const std::string &car_path,
                     std::shared_ptr<Index> index,
                     IpldPtr ipld)
      : car_file{car_path, std::ios::binary}, index{index}, ipld{ipld} {}

  outcome::result<bool> CidsIpld::contains(const CID &cid) const {
    if (auto key{asBlake(cid)}) {
      OUTCOME_TRY(row, index->find(*key));
      if (row) {
        return true;
      }
    }
    if (ipld) {
      return ipld->contains(cid);
    }
    return false;
  }

  outcome::result<void> CidsIpld::set(const CID &cid, Buffer value) {
    OUTCOME_TRY(has, contains(cid));
    if (has) {
      return outcome::success();
    }
    if (ipld) {
      return ipld->set(cid, std::move(value));
    }
    return ERROR_TEXT("CidsIpld.set: no ipld set");
  }

  outcome::result<Buffer> CidsIpld::get(const CID &cid) const {
    if (auto key{asBlake(cid)}) {
      OUTCOME_TRY(row, index->find(*key));
      if (row) {
        std::unique_lock lock{mutex};
        auto [good, size]{readCarItem(car_file, *row, nullptr)};
        if (!good) {
          return ERROR_TEXT("CidsIpld.get: inconsistent");
        }
        Buffer item;
        item.resize(size);
        if (!common::read(car_file, item)) {
          return ERROR_TEXT("CidsIpld.get: read error");
        }
        return item;
      }
    }
    if (ipld) {
      return ipld->get(cid);
    }
    return ipfs::IpfsDatastoreError::kNotFound;
  }
}  // namespace fc::storage::cids_index
