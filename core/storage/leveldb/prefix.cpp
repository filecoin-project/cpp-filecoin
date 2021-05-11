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
    cursor->seek(map._key(key));
  }

  void MapPrefix::Cursor::seekToLast() {
    auto &key{map._next};
    if (key.size() < map.prefix.size()) {
      key = map.prefix;
      // increment
      auto carry{1u};
      auto &_key{key.toVector()};
      for (auto it{_key.rbegin()}; carry && it != _key.rend(); ++it) {
        carry += *it;
        *it = carry & 0xFF;
        carry >>= 8;
      }
      if (carry) {
        key = Buffer(map.prefix.size(), 0xFF);
      }
    }
    if (!key.empty()) {
      cursor->seek(key);
      if (cursor->isValid()) {
        cursor->prev();
        return;
      }
    }
    cursor->seekToLast();
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

  MapPrefix::Batch::Batch(MapPrefix &map, std::unique_ptr<BufferBatch> batch)
      : map{map}, batch{std::move(batch)} {}

  outcome::result<void> MapPrefix::Batch::put(const Buffer &key,
                                              const Buffer &value) {
    return batch->put(map._key(key), value);
  }

  outcome::result<void> MapPrefix::Batch::put(const Buffer &key,
                                              Buffer &&value) {
    return batch->put(map._key(key), std::move(value));
  }

  outcome::result<void> MapPrefix::Batch::remove(const Buffer &key) {
    return batch->remove(map._key(key));
  }

  outcome::result<void> MapPrefix::Batch::commit() {
    return batch->commit();
  }

  void MapPrefix::Batch::clear() {
    batch->clear();
  }

  MapPrefix::MapPrefix(BytesIn prefix, MapPtr map) : prefix{prefix}, map{map} {}

  MapPrefix::MapPrefix(std::string_view prefix, MapPtr map)
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
    return std::make_unique<Batch>(*this, map->batch());
  }

  std::unique_ptr<BufferMapCursor> MapPrefix::cursor() {
    return std::make_unique<Cursor>(*this, map->cursor());
  }

  OneKey::OneKey(BytesIn key, MapPtr map) : key{key}, map{map} {}

  OneKey::OneKey(std::string_view key, MapPtr map)
      : OneKey{common::span::cbytes(key), map} {}

  bool OneKey::has() const {
    return map->contains(key);
  }

  Buffer OneKey::get() const {
    return map->get(key).value();
  }

  void OneKey::set(Buffer value) {
    map->put(key, std::move(value)).value();
  }

  void OneKey::remove() {
    map->remove(key).value();
  }
}  // namespace fc::storage
