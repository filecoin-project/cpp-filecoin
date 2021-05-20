/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

#include "cbor_blake/cid.hpp"
#include "common/buffer.hpp"

namespace fc::crypto::blake2b {
  common::Hash256 blake2b_256(BytesIn);
}  // namespace fc::crypto::blake2b

namespace fc {
  struct CbIpld {
    virtual ~CbIpld() = default;
    virtual bool _get(const CbCid &key, Buffer *value) const = 0;
    virtual void _put(const CbCid &key, BytesIn value) = 0;

    bool has(const CbCid &key) const {
      return _get(key, nullptr);
    }
    bool get(const CbCid &key, Buffer &value) const {
      return _get(key, &value);
    }
    void put(const CbCid &key, BytesIn cbor) {
      return _put(key, cbor);
    }
    CbCid put(BytesIn cbor) {
      auto key{crypto::blake2b::blake2b_256(cbor)};
      _put(key, cbor);
      return key;
    }
  };
  using CbIpldPtr = std::shared_ptr<CbIpld>;
}  // namespace fc
