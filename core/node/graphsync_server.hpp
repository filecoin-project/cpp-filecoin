/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "fwd.hpp"

namespace fc::sync {

  // Graphsync default (IPLD) service handler + engine startup
  class GraphsyncServer {
   public:
    using Graphsync = storage::ipfs::graphsync::Graphsync;

    GraphsyncServer(std::shared_ptr<Graphsync> graphsync, IpldPtr ipld);

    void start();

   private:
    std::shared_ptr<Graphsync> graphsync_;
    IpldPtr ipld_;
    bool started_ = false;

    // TODO (artem):
    // 0) selectors and true IPLD backend
    // 1) request handling in dedicated thread with separate read-only
    // storage access (and RS_TRY_AGAIN replies if queue overloaded)
    // 2) Response caching (hash(request fields)) -> response
  };
}  // namespace fc::sync
