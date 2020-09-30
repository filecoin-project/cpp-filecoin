/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/host/host.hpp>
#include <libp2p/protocol/identify/identify.hpp>
#include <libp2p/protocol/identify/identify_delta.hpp>
#include <libp2p/protocol/identify/identify_push.hpp>

#include "node/hello.hpp"
#include "node/peermgr.hpp"

#define MOVE(x)  \
  x {            \
    std::move(x) \
  }

namespace fc::peermgr {
  PeerMgr::PeerMgr(std::shared_ptr<Host> host,
                   std::shared_ptr<Identify> identify,
                   std::shared_ptr<IdentifyPush> identify_push,
                   std::shared_ptr<IdentifyDelta> identify_delta,
                   std::shared_ptr<Hello> hello) {
    auto handle{[&](auto &protocol) {
      protocol->start();
      host->setProtocolHandler(
          protocol->getProtocolId(),
          [protocol](auto stream) { protocol->handle(std::move(stream)); });
    }};
    handle(identify);
    handle(identify_push);
    handle(identify_delta);
    identify_sub = identify->onIdentifyReceived([MOVE(hello)](auto &peer) {
      hello->say({peer, {}});
    });
  }
}  // namespace fc::peermgr
