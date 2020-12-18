/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/leveldb/prefix.hpp"

#include "common/span.hpp"

namespace fc::storage {
  MapPrefix::MapPrefix(BytesIn prefix, std::shared_ptr<BufferMap> map)
      : prefix{prefix}, map{map} {}

  MapPrefix::MapPrefix(std::string_view prefix, std::shared_ptr<BufferMap> map)
      : MapPrefix{common::span::cbytes(prefix), map} {}

  Buffer MapPrefix::_key(BytesIn key) const {
    return Buffer{prefix}.put(key);
  }

  outcome::result<Buffer> MapPrefix::get(const Buffer &key) const {
    return map->get(_key(key));
  }

  bool MapPrefix::contains(const Buffer &key) const {
    return map->contains(_key(key));
  }

  outcome::result<void> MapPrefix::put(const Buffer &key, const Buffer &value) {
    return map->put(_key(key), value);
  }

  outcome::result<void> MapPrefix::put(const Buffer &key, Buffer &&value) {
    return map->put(_key(key), std::move(value));
  }

  outcome::result<void> MapPrefix::remove(const Buffer &key) {
    return map->remove(_key(key));
  }

  std::unique_ptr<BufferBatch> MapPrefix::batch() {
    throw "not implemented";
  }

  std::unique_ptr<BufferMapCursor> MapPrefix::cursor() {
    throw "not implemented";
  }
}  // namespace fc::storage
