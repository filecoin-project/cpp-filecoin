/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_CAR_CAR_HPP
#define CPP_FILECOIN_CORE_STORAGE_CAR_CAR_HPP

#include "storage/ipfs/datastore.hpp"

namespace fc::storage::car {
  using Ipld = ipfs::IpfsDatastore;
  using Input = gsl::span<const uint8_t>;
  using common::Buffer;

  enum class CarError { DECODE_ERROR = 1 };

  struct CarHeader {
    static constexpr uint64_t V1 = 1;

    std::vector<CID> roots;
    uint64_t version;
  };
  CBOR_ENCODE(CarHeader, header) {
    auto m = s.map();
    m["roots"] << header.roots;
    m["version"] << header.version;
    return s << m;
  }
  CBOR_DECODE(CarHeader, header) {
    auto m = s.map();
    m.at("roots") >> header.roots;
    m.at("version") >> header.version;
    return s;
  }

  outcome::result<std::vector<CID>> loadCar(Ipld &store, Input input);

  void writeHeader(Buffer &output, const std::vector<CID> &roots);

  void writeItem(Buffer &output, const CID &cid, Input bytes);

  outcome::result<Buffer> makeCar(Ipld &store, const std::vector<CID> &roots);
}  // namespace fc::storage::car

OUTCOME_HPP_DECLARE_ERROR(fc::storage::car, CarError);

#endif  // CPP_FILECOIN_CORE_STORAGE_CAR_CAR_HPP
