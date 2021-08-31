/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/async.hpp"
#include "drand/messages.hpp"
#include "fwd.hpp"

namespace fc::drand::http {
  using boost::asio::io_context;

  void getInfo(io_context &io, std::string host, CbT<ChainInfo> cb);

  void getEntry(io_context &io,
                const std::string &host,
                uint64_t round,
                CbT<PublicRandResponse> cb);
}  // namespace fc::drand::http
