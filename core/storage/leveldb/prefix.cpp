/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/leveldb/prefix.hpp"

#include "common/span.hpp"

namespace fc::storage {
  MapPrefix::Cursor::Cursor(MapPrefix &map,
                            std::unique_ptr<BufferMapCursor> cursor)
      : map{map}, cursor{std::move(cursor)} {}

  void MapPrefix::Cursor::seekToFirst() {
    cursor->seek(map.prefix);
  }

  void MapPrefix::Cursor::seek(const Buffer &key) {
    cursor->seek(Buffer{map.prefix}.put(key));
  }

  void MapPrefix::Cursor::seekToLast() {
    throw "not implemented";
  }

  bool MapPrefix::Cursor::isValid() const {
    if (!cursor->isValid()) {
      return false;
    }
    auto key{cursor->key()};
    return key.size() >= map.prefix.size()
           && std::equal(map.prefix.begin(), map.prefix.end(), key.begin());
  }

  void MapPrefix::Cursor::next() {
    assert(isValid());
    cursor->next();
  }

  void MapPrefix::Cursor::prev() {
    assert(isValid());
    cursor->prev();
  }

  Buffer MapPrefix::Cursor::key() const {
    return cursor->key().subbuffer(map.prefix.size());
  }

  Buffer MapPrefix::Cursor::value() const {
    return cursor->value();
  }

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
    return std::make_unique<Cursor>(*this, map->cursor());
  }
}  // namespace fc::storage
