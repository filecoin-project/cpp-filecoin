/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/buffer_map.hpp"

#include <gmock/gmock.h>

namespace fc::storage {
  class BufferMapMock : public BufferMap {
   public:
    MOCK_METHOD0(cursor, std::unique_ptr<face::MapCursor<Buffer, Buffer>>());

    MOCK_CONST_METHOD1(get, outcome::result<Buffer>(const Buffer &));

    MOCK_CONST_METHOD1(contains, bool(const Buffer &));

    MOCK_METHOD2(put, outcome::result<void>(const Buffer &, const Buffer &));

    outcome::result<void> put(const Buffer &key, Buffer &&value) override {
      return doPut(key, value);
    }
    MOCK_METHOD2(doPut, outcome::result<void>(const Buffer &, Buffer));

    MOCK_METHOD1(remove, outcome::result<void>(const Buffer &key));
  };
}  // namespace fc::storage
