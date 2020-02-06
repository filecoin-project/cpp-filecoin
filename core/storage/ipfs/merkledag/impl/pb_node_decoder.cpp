/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/ipfs/merkledag/impl/pb_node_decoder.hpp"

namespace fc::storage::ipfs::merkledag {
  outcome::result<void> PBNodeDecoder::decode(gsl::span<const uint8_t> input) {
    const uint8_t *raw_bytes = input.data();
    if (pb_node_.ParseFromArray(raw_bytes, input.size())) {
      return outcome::success();
    }
    return PBNodeDecodeError::INVALID_RAW_BYTES;
  }

  const std::string &PBNodeDecoder::getContent() const {
    return pb_node_.data();
  }

  size_t PBNodeDecoder::getLinksCount() const {
    return pb_node_.links_size();
  }

  const std::string &PBNodeDecoder::getLinkName(size_t index) const {
    return pb_node_.links(index).name();
  }

  const std::string &PBNodeDecoder::getLinkCID(size_t index) const {
    return pb_node_.links(index).hash();
  }

  size_t PBNodeDecoder::getLinkSize(size_t index) const {
    int size = pb_node_.links(index).tsize();
    return size < 0 ? 0 : static_cast<size_t>(size);
  }
}  // namespace fc::storage::ipfs::merkledag

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::ipfs::merkledag,
                            PBNodeDecodeError,
                            error) {
  using fc::storage::ipfs::merkledag::PBNodeDecodeError;
  switch (error) {
    case (PBNodeDecodeError::INVALID_RAW_BYTES):
      return "IPLD node Protobuf decoder: failed to deserialize from incorrect "
             "raw bytes";
  }
  return "IPLD node protobuf decoder: unknown error";
}
