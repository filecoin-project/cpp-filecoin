/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <set>
#include <string>

#include "codec/cbor/streams_annotation.hpp"
#include "common/buffer.hpp"
#include "primitives/cid/cid.hpp"

namespace fc::storage::ipfs::graphsync {

  constexpr std::string_view kResponseMetadataProtocol =
      "graphsync/response-metadata";
  constexpr std::string_view kDontSendCidsProtocol =
      "graphsync/do-not-send-cids";

  /**
   * ExtensionData is a name/data pair for a graphsync extension
   */
  struct Extension {
    std::string name;
    std::vector<uint8_t> data;

    inline bool operator==(const Extension &other) const {
      return name == other.name && data == other.data;
    }

    static inline boost::optional<BytesIn> find(
        std::string_view name, const std::vector<Extension> &xs) {
      auto it = std::find_if(
          xs.begin(), xs.end(), [&](auto &x) { return x.name == name; });
      if (it == xs.end()) {
        return boost::none;
      }
      return BytesIn{it->data};
    }
  };

  struct ResMeta {
    CID cid;
    bool present;
  };
  CBOR2_DECODE_ENCODE(ResMeta)

  /// Response metadata indicates whether the responder has certain CIDs
  using ResponseMetadata = std::vector<ResMeta>;
}  // namespace fc::storage::ipfs::graphsync
