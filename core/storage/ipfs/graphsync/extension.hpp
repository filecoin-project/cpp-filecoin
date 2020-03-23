/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_EXTENSION_HPP
#define CPP_FILECOIN_GRAPHSYNC_EXTENSION_HPP

#include <string>
#include <set>

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
  };

  /// Returns "graphsync/response-metadata":true extension
  Extension encodeMetadataRequest();

  /// Response metadata indicates whether the responder has certain CIDs
  using ResponseMetadata = std::vector<std::pair<CID, bool>>;

  /// Encodes response metadata pairs
  Extension encodeResponseMetadata(const ResponseMetadata& metadata);

  /// Decodes metadata pairs
  outcome::result<ResponseMetadata> decodeResponseMetadata(
      const Extension &extension);

  /// Encodes CIDS for "graphsync/do-not-send-cids" extension
  Extension encodeDontSendCids(const std::vector<CID> &dont_send_cids);

  /// Decodes CID subset not to be included into response
  outcome::result<std::set<CID>> decodeDontSendCids(
      const Extension &extension);

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_EXTENSION_HPP
