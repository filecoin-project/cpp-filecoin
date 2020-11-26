/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_CAR_CAR_HPP
#define CPP_FILECOIN_CORE_STORAGE_CAR_CAR_HPP

#include "storage/ipfs/datastore.hpp"
#include "storage/ipld/selector.hpp"

namespace fc::storage::car {
  using Ipld = ipfs::IpfsDatastore;
  using Input = gsl::span<const uint8_t>;
  using common::Buffer;
  using ipld::Selector;

  enum class CarError {
    kDecodeError = 1,
    kCannotOpenFileError,
  };

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

  outcome::result<std::vector<CID>> loadCar(Ipld &store,
                                            const std::string &car_path);

  outcome::result<std::vector<CID>> loadCar(Ipld &store, Input input);

  outcome::result<Buffer> makeCar(Ipld &store, const std::vector<CID> &roots);

  outcome::result<Buffer> makeSelectiveCar(
      Ipld &store, const std::vector<std::pair<CID, Selector>> &dags);

  outcome::result<void> makeSelectiveCar(
      Ipld &store,
      const std::vector<std::pair<CID, Selector>> &dags,
      const std::string &output_path);
}  // namespace fc::storage::car

OUTCOME_HPP_DECLARE_ERROR(fc::storage::car, CarError);

#endif  // CPP_FILECOIN_CORE_STORAGE_CAR_CAR_HPP
