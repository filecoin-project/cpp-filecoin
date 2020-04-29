/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/car/selective_car.hpp"
#include "codec/cbor/cbor.hpp"
#include "storage/car/car.hpp"

namespace fc::storage::car {

  outcome::result<Buffer> makeSelectiveCar(
      std::shared_ptr<MerkleDagService> dag_service,
      const std::vector<DAG> &dags) {
    Buffer output;

    // make header
    CarHeader car_header;
    car_header.roots.reserve(dags.size());
    for (const auto &dag : dags) {
      car_header.roots.push_back(dag.root);
    }
    car_header.version = CarHeader::V1;

    OUTCOME_TRY(header_bytes, codec::cbor::encode(car_header));
    output.put(header_bytes);

    bool res = true;
    for (const auto &dag : dags) {
      OUTCOME_TRY(root_bytes, dag.root.toBytes());
      dag_service->select(
          root_bytes,
          dag.selector->getRawBytes(),
          [&output, &res](std::shared_ptr<const IPLDNode> node) {
            auto maybe_cid_bytes = codec::cbor::encode(node->getCID());
            if (maybe_cid_bytes.has_error()) {
              res = false;
              return res;
            }
            output.put(maybe_cid_bytes.value());
            output.put(node->getRawBytes());
            return res;
          });
    }

    if (!res) {
      return CarError::DECODE_ERROR;
    }
    return output;
  }

}  // namespace fc::storage::car
