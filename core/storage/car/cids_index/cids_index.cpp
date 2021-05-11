/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/car/cids_index/cids_index.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/filesystem/operations.hpp>

#include "codec/uvarint.hpp"
#include "common/error_text.hpp"
#include "common/file.hpp"
#include "common/from_span.hpp"
#include "common/logger.hpp"
#include "common/outcome_fmt.hpp"
#include "common/ptr.hpp"
#include "storage/car/car.hpp"
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
    auto size{(uint64_t)_size};
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

  std::pair<bool, size_t> readCarItem(std::istream &car_file,
                                      const Row &row,
                                      uint64_t *end) {
    car_file.clear();
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
    if (!common::write(out, gsl::make_span(&kHeaderV0, 1))) {
      return write_error;
    }
    while (!ranges.empty()) {
      std::pop_heap(ranges.begin(), ranges.end(), cmp);
      auto &range{ranges.back()};
      if (!common::write(out, gsl::make_span(&range.front(), 1))) {
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
    if (!common::write(out, gsl::make_span(&kTrailerV0, 1))) {
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
    if (!common::write(rows_file, gsl::make_span(&kHeaderV0, 1))) {
      return write_error;
    }
    size_t offset{car_min};
    car_file.clear();
    car_file.seekg(offset);
    size_t total{};
    Buffer item;
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
        OUTCOME_TRY(key, fromSpan<Key>(input, false));
        auto &row{rows.emplace_back()};
        row.key = key;
        row.offset = offset;
        row.max_size64 = maxSize64(size);
        ++total;
      } else {
        OUTCOME_TRY(cid, CID::read(input));
        if (ipld) {
          if (!asIdentity(cid)) {
            OUTCOME_TRY(ipld->set(cid, Buffer{input}));
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
    if (!common::write(rows_file, gsl::make_span(&kTrailerV0, 1))) {
      return write_error;
    }
    rows_file.flush();
    return total;
  }

  inline boost::optional<size_t> sparseSize(
      size_t count, boost::optional<size_t> max_memory) {
    if (max_memory && count * sizeof(Row) > *max_memory) {
      return *max_memory / sizeof(Key);
    }
    return boost::none;
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

  inline boost::optional<Row> findWritten(const CidsIpld &ipld,
                                          const Key &key) {
    assert(ipld.writable.is_open());
    auto it{ipld.written.lower_bound(Row{key, {}, {}})};
    if (it != ipld.written.end() && it->key == key) {
      return *it;
    }
    return boost::none;
  }

  outcome::result<bool> CidsIpld::contains(const CID &cid) const {
    if (auto key{asBlake(cid)}) {
      if (has(*key)) {
        return true;
      }
    }
    if (ipld) {
      return ipld->contains(cid);
    }
    return false;
  }

  outcome::result<void> CidsIpld::set(const CID &cid, Buffer value) {
    if (auto key{asBlake(cid)}) {
      if (writable.is_open()) {
        put(*key, value);
        return outcome::success();
      }
    }
    if (ipld) {
      OUTCOME_TRY(has, ipld->contains(cid));
      if (has) {
        return outcome::success();
      }
      return ipld->set(cid, std::move(value));
    }
    return ERROR_TEXT("CidsIpld.set: no ipld set");
  }

  outcome::result<Buffer> CidsIpld::get(const CID &cid) const {
    if (auto key{asBlake(cid)}) {
      Buffer value;
      if (get(*key, value)) {
        return std::move(value);
      }
    }
    if (ipld) {
      return ipld->get(cid);
    }
    return ipfs::IpfsDatastoreError::kNotFound;
  }

  Outcome<void> doFlush(CidsIpld &ipld) {
    std::shared_lock written_slock{ipld.written_mutex};
    uint64_t max_offset{};
    std::vector<Row> rows;
    rows.reserve(ipld.written.size());
    for (auto &row : ipld.written) {
      max_offset = std::max(max_offset, row.offset.value());
      rows.push_back(row);
    }
    written_slock.unlock();
    std::sort(rows.begin(), rows.end());

    std::ifstream index_in{ipld.index_path, std::ios::binary};
    std::vector<MergeRange> ranges;
    auto &range1{ranges.emplace_back()};
    range1.begin = 1;
    range1.end = 1 + ipld.index->size();
    range1.file = &index_in;
    auto &range2{ranges.emplace_back()};
    range2.current = 0;
    range2.rows = std::move(rows);
    auto tmp_path{ipld.index_path + ".tmp"};
    std::ofstream index_out{tmp_path, std::ios::binary};
    OUTCOME_TRY(merge(index_out, std::move(ranges)));

    OUTCOME_TRY(index, load(tmp_path, ipld.max_memory));
    std::unique_lock index_lock{ipld.index_mutex};
    boost::system::error_code ec;
    boost::filesystem::rename(tmp_path, ipld.index_path, ec);
    if (ec) {
      return ec;
    }
    ipld.index = index;
    index_lock.unlock();

    std::unique_lock written_ulock{ipld.written_mutex};
    for (auto it{ipld.written.begin()}; it != ipld.written.end();) {
      if (it->offset.value() > max_offset) {
        ++it;
      } else {
        it = ipld.written.erase(it);
      }
    }
    written_ulock.unlock();

    ipld.flushing.clear();

    return outcome::success();
  }

  bool CidsIpld::get(const Hash256 &key, Buffer *value) const {
    if (value) {
      value->resize(0);
    }
    std::shared_lock index_lock{index_mutex};
    auto row{index->find(key).value()};
    index_lock.unlock();
    if (!row && writable.is_open()) {
      std::shared_lock written_lock{written_mutex};
      row = findWritten(*this, key);
    }
    if (!row) {
      return false;
    }
    if (value) {
      std::unique_lock car_lock{car_mutex};
      auto [good, size]{readCarItem(car_file, *row, nullptr)};
      if (!good) {
        spdlog::error("CidsIpld.get inconsistent");
        outcome::raise(ERROR_TEXT("CidsIpld.get: inconsistent"));
      }
      value->resize(size);
      if (!common::read(car_file, *value)) {
        spdlog::error("CidsIpld.get read error");
        outcome::raise(ERROR_TEXT("CidsIpld.get: read error"));
      }
    }
    return true;
  }

  void CidsIpld::put(const Hash256 &key, BytesIn value) {
    if (!writable.is_open()) {
      outcome::raise(ERROR_TEXT("CidsIpld.put: not writable"));
    }
    if (has(key)) {
      return;
    }
    std::unique_lock written_lock{written_mutex};
    if (findWritten(*this, key)) {
      return;
    }

    Buffer item;
    codec::uvarint::VarintEncoder varint{kCborBlakePrefix.size() + key.size()
                                         + value.size()};
    item.reserve(varint.length + varint.value);
    item.put(varint.bytes());
    item.put(kCborBlakePrefix);
    item.put(key);
    item.put(value);

    Row row;
    row.key = key;
    row.offset = car_offset;
    row.max_size64 = maxSize64(item.size());
    if (!common::write(writable, item)) {
      spdlog::error("CidsIpld.put write error");
      outcome::raise(ERROR_TEXT("CidsIpld.put: write error"));
    }
    if (!writable.flush().good()) {
      spdlog::error("CidsIpld.put flush error");
      outcome::raise(ERROR_TEXT("CidsIpld.put: flush error"));
    }
    car_offset += item.size();
    written.insert(row);
    if (flush_on && written.size() >= flush_on) {
      written_lock.unlock();
      asyncFlush();
    }
  }

  void CidsIpld::asyncFlush() {
    if (!flushing.test_and_set()) {
      if (io) {
        io->post(weakCb0(weak_from_this(), [this] {
          if (auto r{doFlush(*this)}; !r) {
            spdlog::error("CidsIpld({}) async flush: {:#}", index_path, ~r);
          }
        }));
      } else {
        if (auto r{doFlush(*this)}; !r) {
          spdlog::error("CidsIpld({}) flush: {:#}", index_path, ~r);
        }
      }
    }
  }

  outcome::result<bool> Ipld2Ipld::contains(const CID &cid) const {
    return ipld->has(*asBlake(cid));
  }

  outcome::result<void> Ipld2Ipld::set(const CID &cid, Buffer value) {
    ipld->put(*asBlake(cid), value);
    return outcome::success();
  }

  outcome::result<Buffer> Ipld2Ipld::get(const CID &cid) const {
    Buffer value;
    if (!ipld->get(*asBlake(cid), value)) {
      return ipfs::IpfsDatastoreError::kNotFound;
    }
    return std::move(value);
  }
}  // namespace fc::storage::cids_index
