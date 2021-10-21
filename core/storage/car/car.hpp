/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/filesystem.hpp>

#include "storage/ipfs/datastore.hpp"
#include "storage/ipld/selector.hpp"

namespace fc::storage::car {
  using Input = gsl::span<const uint8_t>;
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

  struct CarReader {
    using Item = std::pair<CID, BytesIn>;

    static outcome::result<CarReader> make(BytesIn file);
    bool end() const;
    outcome::result<Item> next();

    BytesIn file;
    std::vector<CID> roots;
    size_t position{}, objects{};
  };

  outcome::result<std::vector<CID>> readHeader(const std::string &car_path);

  /**
   * Loads car to ipld
   * @return root cids for data loaded
   */
  outcome::result<std::vector<CID>> loadCar(Ipld &store,
                                            const std::string &car_path);

  /**
   * Loads car to ipld
   * @return root cids for data loaded
   */
  outcome::result<std::vector<CID>> loadCar(
      Ipld &store, const boost::filesystem::path &car_path);

  outcome::result<std::vector<CID>> loadCar(Ipld &store, Input input);

  void writeHeader(Bytes &output, const std::vector<CID> &roots);

  void writeItem(Bytes &output, const CID &cid, Input bytes);

  outcome::result<Bytes> makeCar(Ipld &store, const std::vector<CID> &roots);

  outcome::result<Bytes> makeSelectiveCar(
      Ipld &store, const std::vector<std::pair<CID, Selector>> &dags);

  outcome::result<void> makeSelectiveCar(
      Ipld &store,
      const std::vector<std::pair<CID, Selector>> &dags,
      const std::string &output_path);
}  // namespace fc::storage::car

OUTCOME_HPP_DECLARE_ERROR(fc::storage::car, CarError);
