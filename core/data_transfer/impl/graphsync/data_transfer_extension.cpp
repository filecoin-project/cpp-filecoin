/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "data_transfer_extension.hpp"
#include "codec/cbor/cbor.hpp"

namespace fc::data_transfer {

  using libp2p::peer::PeerId;

  PeerId ExtensionDataTransferData::getInitiator() const {
    return PeerId::fromBase58(initiator).value();
  }

  outcome::result<Extension> encodeDataTransferExtension(
      const ExtensionDataTransferData &data) {
    OUTCOME_TRY(bytes, codec::cbor::encode(data));
    return Extension{.name = std::string(kDataTransferExtensionName),
                     .data = bytes.toVector()};
  }

  /// Decodes Data Transfer graphsync extension
  outcome::result<ExtensionDataTransferData> decodeDataTransferExtension(
      const Extension &extension) {
    if (extension.name != kDataTransferExtensionName) {
      return DataTransferExtensionError::kUnexpectedExtensionName;
    }
    return codec::cbor::decode<ExtensionDataTransferData>(extension.data);
  }

}  // namespace fc::data_transfer

OUTCOME_CPP_DEFINE_CATEGORY(fc::data_transfer, DataTransferExtensionError, e) {
  using fc::data_transfer::DataTransferExtensionError;

  switch (e) {
    case DataTransferExtensionError::kUnexpectedExtensionName:
      return "DataTransferExtensionError: unexpected extension name";
  }

  return "unknown error";
}
