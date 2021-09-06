/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>

#include "common/span.hpp"

namespace fc {
  auto bytesToIstream(BytesIn bytes) {
    return boost::iostreams::stream{boost::iostreams::array_source{
        common::span::bytestr(bytes.data()),
        static_cast<size_t>(bytes.size()),
    }};
  }
}  // namespace fc
