/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <map>

#include "cbor_blake/ipld.hpp"

namespace fc {
  struct MemoryCbIpld : CbIpld {
    using CbIpld::get;
    using CbIpld::put;

    std::map<CbCid, Buffer> map;

    bool get(const CbCid &key, Buffer *value) const override {
      if (const auto it{map.find(key)}; it != map.end()) {
        if (value) {
          *value = it->second;
        }
        return true;
      }
      return false;
    }
    void put(const CbCid &key, BytesIn value) override {
      map.emplace(key, Buffer{value});
    }
  };
}  // namespace fc
