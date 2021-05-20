/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "common/span.hpp"

namespace fc::storage::ipld {
  using common::Hash256;

  /**
   * Light IPFS that uses only Blake CIDs as Hash256 keys.
   */
  struct LightIpld {
    virtual ~LightIpld() = default;
    virtual bool get(const Hash256 &key, Buffer *value) const = 0;
    virtual void put(const Hash256 &key, BytesIn value) = 0;

    inline bool has(const Hash256 &key) const {
      return get(key, nullptr);
    }
    inline bool get(const Hash256 &key, Buffer &value) const {
      return get(key, &value);
    }
  };

  using LightIpldPtr = std::shared_ptr<LightIpld>;
}  // namespace fc::storage::ipld
