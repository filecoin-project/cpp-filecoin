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

      inline Active(std::shared_ptr<Stream> stream,
                    std::weak_ptr<StreamOpenQueue> weak,
                    List::iterator it);
      inline ~Active() override;
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

    inline StreamOpenQueue(std::shared_ptr<Host> host, size_t max_active);

    inline void open(Pending item);

    inline void check();
  };
}  // namespace libp2p::connection
