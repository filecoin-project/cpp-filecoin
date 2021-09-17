/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/ipld/memory_indexed_car.hpp"

#include <boost/filesystem/operations.hpp>

#include "codec/uvarint.hpp"
#include "common/error_text.hpp"
#include "storage/car/car.hpp"

namespace fc {
  outcome::result<std::shared_ptr<MemoryIndexedCar>> MemoryIndexedCar::make(
      const std::string &path, bool append) {
    auto ipld{std::make_shared<MemoryIndexedCar>()};
    ipld->reader.open(path);
    if (!ipld->reader.is_open()) {
      if (!append) {
        return ERROR_TEXT("MemoryIndexedCar::make open error");
      }
      Buffer header;
      storage::car::writeHeader(header, {});
      OUTCOME_TRY(common::writeFile(path, header));
      ipld->reader.open(path);
    }
    ipld->reader.seekg(0, std::ios::end);
    const auto car_size{static_cast<uint64_t>(ipld->reader.tellg())};
    ipld->reader.seekg(0);
    Buffer item;
    auto varint{codec::uvarint::readBytes(ipld->reader, item)};
    if (varint == 0) {
      return ERROR_TEXT("MemoryIndexedCar::make read error");
    }
    uint64_t offset{varint + item.size()};
    OUTCOME_TRY(header, codec::cbor::decode<storage::car::CarHeader>(item));
    ipld->roots = std::move(header.roots);
    while (offset < car_size) {
      varint = codec::uvarint::readBytes(ipld->reader, item);
      // incomplete item or zero padding
      if (varint == 0 || item.size() == 0) {
        break;
      }
      offset += varint + item.size();
      BytesIn input{item};
      OUTCOME_TRY(cid, CID::read(input));
      const auto size{static_cast<size_t>(input.size())};
      ipld->index.emplace(std::move(cid), std::make_pair(offset - size, size));
    }
    ipld->end = offset;
    if (append) {
      if (offset != car_size) {
        boost::filesystem::resize_file(path, offset);
      }
      ipld->writer.open(path, std::ios::app);
    }
    return ipld;
  }

  outcome::result<bool> MemoryIndexedCar::contains(const CID &key) const {
    std::unique_lock lock{mutex};
    return index.find(key) != index.end();
  }

  outcome::result<void> MemoryIndexedCar::set(const CID &key, Value value) {
    std::unique_lock lock{mutex};
    if (!writer.is_open()) {
      return ERROR_TEXT("MemoryIndexedCar is readonly");
    }
    if (index.find(key) == index.end()) {
      OUTCOME_TRY(key_bytes, key.toBytes());
      codec::uvarint::VarintEncoder varint{key_bytes.size() + value.size()};
      Buffer item;
      item.reserve(varint.length + varint.value);
      item.put(varint.bytes());
      item.put(key_bytes);
      item.put(value);
      if (!common::write(writer, BytesIn{item}) || !writer.flush()) {
        return ERROR_TEXT("MemoryIndexedCar::set write error");
      }
      end += item.size();
      index.emplace(key, std::make_pair(end - value.size(), value.size()));
    }
    return outcome::success();
  }

  outcome::result<Buffer> MemoryIndexedCar::get(const CID &key) const {
    std::unique_lock lock{mutex};
    if (const auto it{index.find(key)}; it != index.end()) {
      const auto &[offset, size]{it->second};
      Buffer value;
      value.resize(size);
      reader.clear();
      reader.seekg(offset);
      if (!common::read(reader, gsl::make_span(value))) {
        return ERROR_TEXT("MemoryIndexedCar::get read error");
      }
      return value;
    }
    return storage::ipfs::IpfsDatastoreError::kNotFound;
  }
}  // namespace fc
