/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/buffer_map.hpp"

namespace fc::storage {
  struct MapPrefix : PersistentBufferMap {
    MapPrefix(BytesIn prefix, std::shared_ptr<BufferMap> map);
    MapPrefix(std::string_view prefix, std::shared_ptr<BufferMap> map);
    Buffer _key(BytesIn key) const;

    outcome::result<Buffer> get(const Buffer &key) const override;
    bool contains(const Buffer &key) const override;
    outcome::result<void> put(const Buffer &key, const Buffer &value) override;
    outcome::result<void> put(const Buffer &key, Buffer &&value) override;
    outcome::result<void> remove(const Buffer &key) override;
    std::unique_ptr<BufferBatch> batch() override;
    std::unique_ptr<BufferMapCursor> cursor() override;

    Buffer prefix;
    std::shared_ptr<BufferMap> map;
  };
}  // namespace fc::storage
