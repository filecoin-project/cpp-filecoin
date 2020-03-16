/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_DATA_TRANSFER_EXTENSION_HPP
#define CPP_FILECOIN_DATA_TRANSFER_EXTENSION_HPP

#include <libp2p/peer/peer_info.hpp>
#include "data_transfer/types.hpp"
#include "storage/ipfs/graphsync/extension.hpp"

namespace fc::data_transfer {

  using storage::ipfs::graphsync::Extension;

  constexpr std::string_view kDataTransferExtensionName = "fil/data-transfer";

  /**
   * Data Transfer extension for graphsync protocol
   */
  struct ExtensionDataTransferData {
    TransferId transfer_id;
    std::string initiator;
    bool is_pull;

    PeerInfo getInitiator() const;
  };

  CBOR_TUPLE(ExtensionDataTransferData, transfer_id, initiator, is_pull)

  /// Encodes Data Transfer graphsync extension
  outcome::result<Extension> encodeDataTransferExtension(
      const ExtensionDataTransferData &data);

  /// Decodes Data Transfer graphsync extension
  outcome::result<ExtensionDataTransferData> decodeDataTransferExtension(
      const Extension &extension);

  /**
   * @brief Type of errors returned by ExtensionDataTransferData
   */
  enum class DataTransferExtensionError { UNEXPECTED_EXTENSION_NAME };

}  // namespace fc::data_transfer

OUTCOME_HPP_DECLARE_ERROR(fc::data_transfer, DataTransferExtensionError);

#endif  // CPP_FILECOIN_DATA_TRANSFER_EXTENSION_HPP
