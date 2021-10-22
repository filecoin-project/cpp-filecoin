/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

#include "cbor_blake/cid.hpp"
#include "common/bytes_cow.hpp"

namespace fc {
  struct CbIpld {
    virtual ~CbIpld() = default;
    virtual bool get(const CbCid &key, Bytes *value) const = 0;
    virtual void put(const CbCid &key, BytesCow &&value) = 0;

    bool has(const CbCid &key) const {
      return get(key, nullptr);
    }
    bool get(const CbCid &key, Bytes &value) const {
      return get(key, &value);
    }
    CbCid put(BytesCow &&cbor) {
      auto key{CbCid::hash(cbor)};
      put(key, std::move(cbor));
      return key;
    }
  };
  using CbIpldPtr = std::shared_ptr<CbIpld>;
}  // namespace fc
