/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>

#include "common/span.hpp"

namespace fc {
  struct BytesIstream {
    boost::iostreams::array_source device;
    boost::iostreams::stream<decltype(device)> s;

    explicit BytesIstream(BytesIn bytes)
        : device{common::span::bytestr(bytes.data()),
                 static_cast<size_t>(bytes.size())},
          s{device} {}
  };
}  // namespace fc
