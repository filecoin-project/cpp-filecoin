/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/blob.hpp"
#include "common/buffer.hpp"

namespace fc {
  using common::Hash256;

  struct Ipld2 {
    virtual ~Ipld2() = default;
    virtual bool get(const Hash256 &key, Buffer *value) const = 0;
    virtual void put(const Hash256 &key, BytesIn value) = 0;

    inline bool has(const Hash256 &key) const {
      return get(key, nullptr);
    }
    inline bool get(const Hash256 &key, Buffer &value) const {
      return get(key, &value);
    }
  };
  using Ipld2Ptr = std::shared_ptr<Ipld2>;
}  // namespace fc
