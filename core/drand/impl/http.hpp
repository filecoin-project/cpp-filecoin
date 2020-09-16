/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/async.hpp"
#include "drand/messages.hpp"
#include "node/fwd.hpp"

namespace fc::drand::http {
  using boost::asio::io_context;

  enum class Error {
    kJson = 1,
  };

  void getInfo(io_context &io, std::string host, CbT<ChainInfo> cb);

  void getEntry(io_context &io,
                std::string host,
                uint64_t round,
                CbT<PublicRandResponse> cb);
}  // namespace fc::drand::http

OUTCOME_HPP_DECLARE_ERROR(fc::drand::http, Error);
