/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/ipld/cids_ipld.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/filesystem/operations.hpp>

#include "cbor_blake/ipld_any.hpp"
#include "codec/cbor/light_reader/cid.hpp"
#include "codec/uvarint.hpp"
#include "common/error_text.hpp"
#include "common/logger.hpp"
#include "common/outcome_fmt.hpp"
#include "common/ptr.hpp"

namespace fc::storage::ipld {
  using cids_index::maxSize64;
  using cids_index::MergeRange;

  boost::optional<Row> CidsIpld::findWritten(const CbCid &key) const {
    assert(writable != nullptr);
    auto it{written.lower_bound(Row{key, {}, {}})};
    if (it != written.end() && it->key == key) {
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

  outcome::result<void> CidsIpld::set(const CID &cid, BytesCow &&value) {
    if (auto key{asBlake(cid)}) {
      if (writable != nullptr) {
        put(*key, std::move(value));
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

  outcome::result<Bytes> CidsIpld::get(const CID &cid) const {
    if (auto key{asBlake(cid)}) {
      Bytes value;
      if (get(*key, value)) {
        return std::move(value);
      }
    }
    if (ipld) {
      return ipld->get(cid);
    }
    return ipfs::IpfsDatastoreError::kNotFound;
  }

  Outcome<void> CidsIpld::doFlush() {
    std::unique_lock flush_lock{flush_mutex};
    std::shared_lock written_slock{written_mutex};
    uint64_t max_offset{};
    std::vector<Row> rows;
    rows.reserve(written.size());
    for (const auto &row : written) {
      max_offset = std::max(max_offset, row.offset.value());
      rows.push_back(row);
    }
    written_slock.unlock();
    std::sort(rows.begin(), rows.end());

    std::ifstream index_in{index_path, std::ios::binary};
    std::vector<MergeRange> ranges;
    auto &range1{ranges.emplace_back()};
    range1.begin = 1;
    range1.end = 1 + index->size();
    range1.file = &index_in;
    auto &range2{ranges.emplace_back()};
    range2.current = 0;
    range2.rows = std::move(rows);
    auto tmp_path{index_path + ".tmp"};
    std::ofstream index_out{tmp_path, std::ios::binary};
    OUTCOME_TRY(merge(index_out, std::move(ranges)));

    OUTCOME_TRY(new_index, cids_index::load(tmp_path, max_memory));
    std::unique_lock index_lock{index_mutex};
    boost::system::error_code ec;
    boost::filesystem::rename(tmp_path, index_path, ec);
    if (ec) {
      return ec;
    }
    index = new_index;
    index_lock.unlock();

    std::unique_lock written_ulock{written_mutex};
    for (auto it{written.begin()}; it != written.end();) {
      if (it->offset.value() > max_offset) {
        ++it;
      } else {
        it = written.erase(it);
      }
    }
    written_ulock.unlock();

    flushing.clear();

    return outcome::success();
  }

  bool CidsIpld::get(const CbCid &key, Bytes *value) const {
    if (value != nullptr) {
      value->resize(0);
    }
    std::shared_lock index_lock{index_mutex};
    auto row{index->find(key).value()};
    index_lock.unlock();
    if (!row && writable != nullptr) {
      std::shared_lock written_lock{written_mutex};
      row = findWritten(key);
    }
    if (!row) {
      if (ipld) {
        return AnyAsCbIpld::get(ipld, key, value);
      }
      return false;
    }
    if (value != nullptr) {
      if (carGet(*row, *value)) {
        return true;
      }
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

  void CidsIpld::put(const CbCid &key, BytesCow &&value) {
    if (writable == nullptr) {
      outcome::raise(ERROR_TEXT("CidsIpld.put: not writable"));
    }
    if (has(key)) {
      return;
    }
    std::unique_lock written_lock{written_mutex};
    if (findWritten(key)) {
      return;
    }

    Bytes item;
    codec::uvarint::VarintEncoder varint{kCborBlakePrefix.size() + CbCid::size()
                                         + value.size()};
    item.reserve(varint.length + varint.value);
    append(item, varint.bytes());
    append(item, kCborBlakePrefix);
    append(item, key);
    append(item, value);

    Row row;
    row.key = key;
    row.offset = car_offset;
    row.max_size64 = maxSize64(item.size());
    car_offset += item.size();
    carPut(row, std::move(item));
    written.insert(row);
    if (flush_on != 0 && written.size() >= flush_on) {
      written_lock.unlock();
      asyncFlush();
    }
  }

  void CidsIpld::carPut(const Row &row, Bytes &&item) {
    std::unique_lock lock{car_flush_mutex};
    car_queue.emplace(row.offset.value(), car_queue_buffer.size());
    append(car_queue_buffer, item);
    if (car_queue.size() >= car_flush_on) {
      carFlush(std::adopt_lock);
    }
  }

  bool CidsIpld::carGet(const Row &row, Bytes &value) const {
    std::shared_lock lock{car_flush_mutex};
    const auto it{car_queue.find(row.offset.value())};
    if (it == car_queue.end()) {
      return false;
    }
    auto item{BytesIn{car_queue_buffer}.subspan(it->second)};
    BytesIn input;
    const CbCid *hash;
    if (codec::uvarint::readBytes(input, item)
        && codec::cbor::light_reader::readCborBlake(hash, input)) {
      copy(value, input);
      return true;
    }
    outcome::raise(ERROR_TEXT("CidsIpld.carGet decode error"));
  }

  void CidsIpld::carFlush(std::adopt_lock_t) {
    if (car_queue.empty()) {
      return;
    }
    if (fwrite(
            car_queue_buffer.data(), car_queue_buffer.size(), 1, writable.get())
        != 1) {
      spdlog::error("CidsIpld.carFlush write error");
      outcome::raise(ERROR_TEXT("CidsIpld.carFlush: write error"));
    }
    if (fflush(writable.get()) != 0) {
      spdlog::error("CidsIpld.carFlush flush error");
      outcome::raise(ERROR_TEXT("CidsIpld.carFlush: flush error"));
    }
    car_queue.clear();
    car_queue_buffer.resize(0);
  }

  void CidsIpld::carFlush() {
    std::unique_lock lock{car_flush_mutex};
    carFlush(std::adopt_lock);
  }

  void CidsIpld::asyncFlush() {
    if (!flushing.test_and_set()) {
      if (io) {
        io->post(weakCb(*this, [this](std::shared_ptr<CidsIpld> &&self) {
          if (auto r{doFlush()}; !r) {
            spdlog::error("CidsIpld({}) async flush: {:#}", index_path, ~r);
          }
        }));
      } else {
        if (auto r{doFlush()}; !r) {
          spdlog::error("CidsIpld({}) flush: {:#}", index_path, ~r);
        }
      }
    }
  }
}  // namespace fc::storage::ipld
