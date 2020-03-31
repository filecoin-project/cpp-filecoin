/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/genesis/genesis.hpp"

#include <libp2p/multi/uvarint.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::genesis, CarError, e) {
  using E = fc::storage::genesis::CarError;
  switch (e) {
    case E::DECODE_ERROR:
      return "Decode error";
  }
}

namespace fc::storage::genesis {
  using libp2p::multi::UVarint;

  outcome::result<uint64_t> readUvarint(Input &input) {
    auto value = UVarint::create(input);
    if (!value) {
      return CarError::DECODE_ERROR;
    }
    input = input.subspan(UVarint::calculateSize(input));
    return value->toUInt64();
  }

  outcome::result<Input> readUvarintBytes(Input &input) {
    OUTCOME_TRY(size, readUvarint(input));
    if (input.size() < static_cast<ptrdiff_t>(size)) {
      return CarError::DECODE_ERROR;
    }
    auto result = input.subspan(0, size);
    input = input.subspan(size);
    return result;
  }

  outcome::result<CID> readCid(Input &input) {
    auto input2 = input;
    if (input.size() >= 2 && input[0] == 12 && input[1] == 32) {
      if (input.size() < 34) {
        return CarError::DECODE_ERROR;
      }
    } else {
      OUTCOME_TRY(readUvarint(input));
      OUTCOME_TRY(readUvarint(input));
    }
    OUTCOME_TRY(readUvarint(input));
    OUTCOME_TRY(readUvarintBytes(input));
    OUTCOME_TRY(
        cid, CID::fromBytes(input2.subspan(0, input2.size() - input.size())));
    return std::move(cid);
  }

  outcome::result<std::vector<CID>> loadCar(Ipld &store, Input input) {
    OUTCOME_TRY(header_bytes, readUvarintBytes(input));
    OUTCOME_TRY(header, codec::cbor::decode<CarHeader>(header_bytes));
    while (!input.empty()) {
      OUTCOME_TRY(node, readUvarintBytes(input));
      OUTCOME_TRY(cid, readCid(node));
      OUTCOME_TRY(store.set(cid, common::Buffer{node}));
    }
    return std::move(header.roots);
  }
}  // namespace fc::storage::genesis
