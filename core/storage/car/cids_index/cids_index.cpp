/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/car/cids_index/cids_index.hpp"

#include "codec/uvarint.hpp"
#include "common/error_text.hpp"
#include "common/file.hpp"
#include "common/from_span.hpp"
#include "common/logger.hpp"
#include "common/ptr.hpp"
#include "storage/car/cids_index/progress.hpp"
#include "storage/ipfs/ipfs_datastore_error.hpp"

namespace fc::storage::cids_index {
  outcome::result<size_t> checkIndex(std::ifstream &file) {
    file.seekg(0, std::ios::end);
    auto _size{file.tellg()};
    if (_size < 0) {
      return ERROR_TEXT("checkIndex: get file size failed");
    }
    auto size{(uint64_t)_size};
    if (size % sizeof(Row) != 0) {
      return ERROR_TEXT("checkIndex: invalid file size");
    }
    file.seekg(0);
    Row header;
    if (!common::readStruct(file, header)) {
      return ERROR_TEXT("checkIndex: read header failed");
    }
    if (header != kHeaderV0) {
      return ERROR_TEXT("checkIndex: invalid header");
    }
    file.seekg(-sizeof(Row), std::ios::end);
    Row trailer;
    if (!common::readStruct(file, trailer)) {
      return ERROR_TEXT("checkIndex: read trailer failed");
    }
    if (trailer != kTrailerV0) {
      return ERROR_TEXT("checkIndex: invalid trailer");
    }
    file.seekg(sizeof(Row));
    return size / sizeof(Row) - 2;
  }

  std::pair<bool, size_t> readCarItem(std::istream &car_file,
                                      const Row &row,
                                      uint64_t *end) {
    car_file.clear();
    car_file.seekg(row.offset.value());
    auto prefix{kCborBlakePrefix};
    CbCid key;
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

  bool MergeRange::empty() const {
    return current >= rows.size() && begin >= end;
  }

  bool MergeRange::read() {
    assert(!empty());
    if (current >= rows.size()) {
      file->seekg(begin * sizeof(Row));
      rows.resize(std::min(rows.size(), end - begin));
      if (!common::read(*file, gsl::make_span(rows))) {
        return false;
      }
      begin += rows.size();
      current = 0;
    }
    return true;
  }

  void MergeRange::pop() {
    assert(current < rows.size());
    ++current;
  }

  outcome::result<void> merge(std::ostream &out,
                              std::vector<MergeRange> &&ranges) {
    auto read_error{ERROR_TEXT("merge: read error")};
    auto write_error{ERROR_TEXT("merge: write error")};
    std::greater<MergeRange> cmp;
    for (auto it{ranges.begin()}; it != ranges.end();) {
      auto &range{*it};
      if (range.empty()) {
        it = ranges.erase(it);
      } else {
        if (range.file) {
          // estimated, 64kb
          range.rows.resize(1638);
          if (!range.read()) {
            return read_error;
          }
        }
        ++it;
      }
    }
    std::make_heap(ranges.begin(), ranges.end(), cmp);
    if (!common::writeStruct(out, kHeaderV0)) {
      return write_error;
    }
    while (!ranges.empty()) {
      std::pop_heap(ranges.begin(), ranges.end(), cmp);
      auto &range{ranges.back()};
      if (!common::writeStruct(out, range.front())) {
        return write_error;
      }
      range.pop();
      if (range.empty()) {
        ranges.pop_back();
      } else {
        if (!range.read()) {
          return read_error;
        }
        std::push_heap(ranges.begin(), ranges.end(), cmp);
      }
    }
    if (!common::writeStruct(out, kTrailerV0)) {
      return write_error;
    }
    out.flush();
    return outcome::success();
  }

  outcome::result<size_t> readCar(std::istream &car_file,
                                  uint64_t car_min,
                                  uint64_t car_max,
                                  boost::optional<size_t> max_memory,
                                  IpldPtr ipld,
                                  Progress *progress,
                                  std::fstream &rows_file,
                                  std::vector<MergeRange> &ranges) {
    assert(car_min <= car_max);
    auto write_error{ERROR_TEXT("readCar: write error")};
    // estimated, 64kb
    car_file.rdbuf()->pubsetbuf(nullptr, 64 << 10);
    std::vector<Row> rows;
    if (max_memory) {
      // estimated, 16mb, 512mb
      rows.reserve(std::clamp<size_t>(*max_memory, 16 << 20, 512 << 20)
                   / sizeof(Row));
    } else {
      // estimated
      rows.reserve((car_max - car_min) * 33 / 23520);
    }
    if (!common::writeStruct(rows_file, kHeaderV0)) {
      return write_error;
    }
    size_t offset{car_min};
    car_file.clear();
    car_file.seekg(offset);
    size_t total{};
    Bytes item;
    auto flush{[&] {
      auto &range{ranges.emplace_back()};
      range.begin = 1 + total - rows.size();
      range.end = 1 + total;
      range.file = &rows_file;
      std::sort(rows.begin(), rows.end());
      auto res{common::write(rows_file, gsl::make_span(rows))};
      rows.resize(0);
      return res;
    }};
    while (offset < car_max) {
      auto varint{codec::uvarint::readBytes(car_file, item)};
      if (!varint) {
        break;
      }
      BytesIn input{item};
      auto size{varint + item.size()};
      if (startsWith(item, kCborBlakePrefix)) {
        input = input.subspan(kCborBlakePrefix.size());
        OUTCOME_TRY(key, fromSpan<CbCid>(input, false));
        auto &row{rows.emplace_back()};
        row.key = key;
        row.offset = offset;
        row.max_size64 = maxSize64(size);
        ++total;
      } else {
        if (!startsWith(input, kMainnetGenesisBlockParent)) {
          OUTCOME_TRY(cid, CID::read(input));
          if (ipld) {
            if (!asIdentity(cid)) {
              OUTCOME_TRY(ipld->set(cid, input));
            }
          }
        }
      }
      offset += size;
      if (max_memory && rows.size() == rows.capacity() && !flush()) {
        return write_error;
      }
      if (progress) {
        progress->car_offset.value = offset - car_min;
        ++progress->items.value;
        progress->update();
      }
    }
    if (!flush()) {
      return write_error;
    }
    if (!common::writeStruct(rows_file, kTrailerV0)) {
      return write_error;
    }
    rows_file.flush();
    return total;
  }

  inline boost::optional<size_t> sparseSize(
      size_t count, boost::optional<size_t> max_memory) {
    if (max_memory && count * sizeof(Row) > *max_memory) {
      return *max_memory / sizeof(CbCid);
    }
    return boost::none;
  }

  outcome::result<boost::optional<Row>> MemoryIndex::find(
      const CbCid &key) const {
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
      const CbCid &key) const {
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
      if (!common::readStruct(index_file, row)) {
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
        if (!common::readStruct(file, row)) {
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

  outcome::result<std::shared_ptr<Index>> load(
      const std::string &index_path, boost::optional<size_t> max_memory) {
    std::ifstream index_file{index_path, std::ios::binary};
    // estimated
    index_file.rdbuf()->pubsetbuf(nullptr, 64 << 10);
    OUTCOME_TRY(count, checkIndex(index_file));
    if (auto sparse{sparseSize(count, max_memory)}) {
      OUTCOME_TRY(index,
                  SparseIndex::load(std::move(index_file), count, *sparse));
      return std::move(index);
    }
    OUTCOME_TRY(index, MemoryIndex::load(index_file, count));
    return std::move(index);
  }
}  // namespace fc::storage::cids_index
