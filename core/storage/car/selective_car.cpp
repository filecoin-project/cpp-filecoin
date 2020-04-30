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

    std::vector<CID> roots;
    roots.reserve(dags.size());
    for (const auto &dag : dags) {
      roots.push_back(dag.root);
    }
    writeHeader(output, roots);

    bool res = true;
    for (const auto &dag : dags) {
      OUTCOME_TRY(root_bytes, dag.root.toBytes());
      OUTCOME_TRY(dag_service->select(
          root_bytes,
          dag.selector->getRawBytes(),
          [&output](std::shared_ptr<const IPLDNode> node) {
            writeItem(output, node->getCID(), node->getRawBytes());
            return true;
          }));
    }

    if (!res) {
      return CarError::DECODE_ERROR;
    }
    return output;
  }

}  // namespace fc::storage::car
