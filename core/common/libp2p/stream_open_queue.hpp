/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/host/host.hpp>
#include <list>
#include <queue>

#include "common/libp2p/stream_proxy.hpp"

namespace libp2p::connection {
  struct StreamOpenQueue : std::enable_shared_from_this<StreamOpenQueue> {
    struct Active : StreamProxy {
      using List = std::list<std::weak_ptr<Active>>;

      std::weak_ptr<StreamOpenQueue> weak;
      List::iterator it;

      Active(const Active &) = delete;
      Active(Active &&) = delete;
      Active(std::shared_ptr<Stream> stream,
             std::weak_ptr<StreamOpenQueue> weak,
             List::iterator it);
      ~Active() override;
      Active &operator=(const Active &) = delete;
      Active &operator=(Active &&) = delete;
    };

    struct Pending {
      peer::PeerInfo peer;
      peer::Protocol protocol;
      Host::StreamResultHandler cb;
    };

    std::shared_ptr<Host> host;
    size_t max_active;
    std::queue<Pending> queue;
    Active::List active;

    StreamOpenQueue(std::shared_ptr<Host> host, size_t max_active);

    void open(Pending item);

    void check();
  };
}  // namespace libp2p::connection
