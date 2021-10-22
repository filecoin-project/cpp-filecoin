/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "cbor_blake/ipld.hpp"

namespace fc {

  class CborBlakeIpldMock : public CbIpld {
   public:
    MOCK_CONST_METHOD2(get, bool(const CbCid &key, Bytes *value));
    MOCK_METHOD2(putMock, void(const CbCid &key, BytesIn value));
    void put(const CbCid &key, BytesCow &&value) override {
      putMock(key, value);
    }
  };
}  // namespace fc
