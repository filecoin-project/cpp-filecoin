/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/car/car.hpp"

#include "codec/uvarint.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::car, CarError, e) {
  using E = fc::storage::car::CarError;
  switch (e) {
    case E::DECODE_ERROR:
      return "Decode error";
  }
}

namespace fc::storage::car {
  outcome::result<std::vector<CID>> loadCar(Ipld &store, Input input) {
    OUTCOME_TRY(header_bytes,
                codec::uvarint::readBytes<CarError::DECODE_ERROR,
                                          CarError::DECODE_ERROR>(input));
    OUTCOME_TRY(header, codec::cbor::decode<CarHeader>(header_bytes));
    while (!input.empty()) {
      OUTCOME_TRY(node,
                  codec::uvarint::readBytes<CarError::DECODE_ERROR,
                                            CarError::DECODE_ERROR>(input));
      OUTCOME_TRY(cid, CID::read(node));
      OUTCOME_TRY(store.set(cid, common::Buffer{node}));
    }
    return std::move(header.roots);
  }
}  // namespace fc::storage::car
