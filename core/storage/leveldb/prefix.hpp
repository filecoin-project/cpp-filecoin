/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/fwd.hpp"
#include "storage/buffer_map.hpp"

namespace fc::storage {
  using MapPtr = std::shared_ptr<PersistentBufferMap>;

  struct MapPrefix : PersistentBufferMap {
    struct Cursor : BufferMapCursor {
      Cursor(MapPrefix &map, std::unique_ptr<BufferMapCursor> cursor);

      void seekToFirst() override;
      void seek(const Buffer &key) override;
      void seekToLast() override;
      bool isValid() const override;
      void next() override;
      void prev() override;
      Buffer key() const override;
      Buffer value() const override;

      MapPrefix &map;
      std::unique_ptr<BufferMapCursor> cursor;
    };

    struct Batch : BufferBatch {
      Batch(MapPrefix &map, std::unique_ptr<BufferBatch> batch);

      outcome::result<void> put(const Buffer &key,
                                const Buffer &value) override;
      outcome::result<void> put(const Buffer &key, Buffer &&value) override;
      outcome::result<void> remove(const Buffer &key) override;

      outcome::result<void> commit() override;
      void clear() override;

      MapPrefix &map;
      std::unique_ptr<BufferBatch> batch;
    };

    MapPrefix(BytesIn prefix, MapPtr map);
    MapPrefix(std::string_view prefix, MapPtr map);
    Buffer _key(BytesIn key) const;

    outcome::result<Buffer> get(const Buffer &key) const override;
    bool contains(const Buffer &key) const override;
    outcome::result<void> put(const Buffer &key, const Buffer &value) override;
    outcome::result<void> put(const Buffer &key, Buffer &&value) override;
    outcome::result<void> remove(const Buffer &key) override;
    std::unique_ptr<BufferBatch> batch() override;
    std::unique_ptr<BufferMapCursor> cursor() override;

    Buffer prefix, _next;
    MapPtr map;
  };

  struct OneKey {
    OneKey(BytesIn key, MapPtr map);
    OneKey(std::string_view key, MapPtr map);
    bool has() const;
    Buffer get() const;
    void set(Buffer value);
    void remove();
    template <typename T>
    auto getCbor() const {
      return codec::cbor::decode<T>(get()).value();
    }
    template <typename T>
    void getCbor(T &value) const {
      value = getCbor<T>();
    }
    template <typename T>
    void setCbor(const T &value) {
      return set(codec::cbor::encode(value).value());
    }

    Buffer key;
    MapPtr map;
  };
}  // namespace fc::storage
