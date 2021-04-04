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

  outcome::result<std::shared_ptr<Index>> load(const std::string &index_path) {
    auto index_size{boost::filesystem::file_size(index_path)};
    if (index_size % sizeof(Row) != 0) {
      return ERROR_TEXT("cids_index load: invalid index file size");
    }
    std::ifstream index_file{index_path, std::ios::binary};
    Row header;
    if (!read(index_file, header)) {
      return ERROR_TEXT("cids_index load: read header failed");
    }
    if (header != kHeaderV0) {
      return ERROR_TEXT("cids_index load: invalid header");
    }
    index_file.seekg(-sizeof(Row), std::ios::end);
    Row trailer;
    if (!read(index_file, trailer)) {
      return ERROR_TEXT("cids_index load: read trailer failed");
    }
    if (trailer != kTrailerV0) {
      return ERROR_TEXT("cids_index load: invalid trailer");
    }

    index_file.seekg(sizeof(Row));
    auto index{std::make_shared<MemoryIndex>()};
    index->rows.resize(index_size / sizeof(Row) - 2);
    if (!common::read(index_file, gsl::make_span(index->rows))) {
      return ERROR_TEXT("cids_index load: read rows failed");
    }
    return index;
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
        car_file.seekg(row->offset.value());
        Buffer item;
        if (codec::uvarint::readBytes(car_file, item)) {
          lock.unlock();
          BytesIn input{item};
          if (startsWith(input, kCborBlakePrefix)) {
            input = input.subspan(kCborBlakePrefix.size());
            if (startsWith(input, row->key)) {
              input = input.subspan(row->key.size());
              memmove(item.data(), input.data(), input.size());
              item.resize(input.size());
              return std::move(item);
            }
          }
          return ERROR_TEXT("CidsIpld.get: inconsistent");
        } else {
          return ERROR_TEXT("CidsIpld.get: read error");
        }
      }
    }
    if (ipld) {
      return ipld->get(cid);
    }
    return ipfs::IpfsDatastoreError::kNotFound;
  }
}  // namespace fc::storage::cids_index
