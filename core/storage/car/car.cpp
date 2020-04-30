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
  using ipld::walker::Walker;

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

  void writeUvarint(Buffer &output, uint64_t value) {
    output.put(libp2p::multi::UVarint{value}.toBytes());
  }

  void writeHeader(Buffer &output, const std::vector<CID> &roots) {
    OUTCOME_EXCEPT(bytes, codec::cbor::encode(CarHeader{roots, CarHeader::V1}));
    writeUvarint(output, bytes.size());
    output.put(bytes);
  }

  void writeItem(Buffer &output, const CID &cid, Input bytes) {
    OUTCOME_EXCEPT(cid_bytes, cid.toBytes());
    writeUvarint(output, cid_bytes.size() + bytes.size());
    output.put(cid_bytes);
    output.put(bytes);
  }

  outcome::result<void> writeItem(Buffer &output, Ipld &store, const CID &cid) {
    OUTCOME_TRY(bytes, store.get(cid));
    writeItem(output, cid, bytes);
    return outcome::success();
  }

  outcome::result<Buffer> makeCar(Ipld &store,
                                  const std::vector<CID> &roots,
                                  const Walker &walker) {
    Buffer output;
    writeHeader(output, roots);
    for (auto &cid : walker.cids) {
      OUTCOME_TRY(writeItem(output, store, cid));
    }
    return std::move(output);
  }

  outcome::result<Buffer> makeCar(Ipld &store, const std::vector<CID> &roots) {
    Walker walker{store};
    for (auto &root : roots) {
      OUTCOME_TRY(walker.recursiveAll(root));
    }
    return makeCar(store, roots, walker);
  }

  outcome::result<Buffer> makeSelectiveCar(
      Ipld &store, const std::vector<std::pair<CID, Selector>> &dags) {
    Walker walker{store};
    std::vector<CID> roots;
    for (auto &dag : dags) {
      OUTCOME_TRY(walker.select(dag.first, dag.second));
      roots.push_back(dag.first);
    }
    return makeCar(store, roots, walker);
  }
}  // namespace fc::storage::car
