/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "data_transfer_extension.hpp"
#include "codec/cbor/cbor.hpp"

namespace fc::data_transfer {

  using libp2p::peer::PeerId;

  PeerInfo ExtensionDataTransferData::getInitiator() const {
    PeerId peer_id = PeerId::fromBase58(initiator).value();
    return PeerInfo{.id = peer_id};
  }

  outcome::result<Extension> encodeDataTransferExtension(
      const ExtensionDataTransferData &data) {
    OUTCOME_TRY(bytes, codec::cbor::encode(data));
    return Extension{.name = std::string(kDataTransferExtensionName),
                     .data = bytes};
  }

  /// Decodes Data Transfer graphsync extension
  outcome::result<ExtensionDataTransferData> decodeDataTransferExtension(
      const Extension &extension) {
    if (extension.name != kDataTransferExtensionName) {
      return DataTransferExtensionError::UNEXPECTED_EXTENSION_NAME;
    }
    return codec::cbor::decode<ExtensionDataTransferData>(extension.data);
  }

}  // namespace fc::data_transfer

OUTCOME_CPP_DEFINE_CATEGORY(fc::data_transfer, DataTransferExtensionError, e) {
  using fc::data_transfer::DataTransferExtensionError;

  switch (e) {
    case DataTransferExtensionError::UNEXPECTED_EXTENSION_NAME:
      return "DataTransferExtensionError: unexpected extension name";
  }

  return "unknown error";
}
