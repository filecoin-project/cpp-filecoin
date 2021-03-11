/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "node/events_fwd.hpp"
#include "primitives/cid/cid.hpp"

namespace fc::sync {

  inline constexpr auto kHelloProtocol = "/fil/hello/1.0.0";

  struct HelloMessage {
    std::vector<CID> heaviest_tipset;
    uint64_t heaviest_tipset_height;
    BigInt heaviest_tipset_weight;
    CID genesis;
  };

  struct LatencyMessage {
    uint64_t arrival, sent;
  };

  CBOR_TUPLE(HelloMessage,
             heaviest_tipset,
             heaviest_tipset_height,
             heaviest_tipset_weight,
             genesis)
  CBOR_TUPLE(LatencyMessage, arrival, sent)

}  // namespace fc::sync
