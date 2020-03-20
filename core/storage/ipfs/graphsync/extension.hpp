/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_EXTENSION_HPP
#define CPP_FILECOIN_GRAPHSYNC_EXTENSION_HPP

#include <string>
#include <vector>

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

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_EXTENSION_HPP
