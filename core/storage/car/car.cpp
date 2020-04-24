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
  using codec::cbor::CborDecodeStream;

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

  struct WriteVisitor {
    outcome::result<void> visit(const CID &cid) {
      if (visited.insert(cid).second) {
        OUTCOME_TRY(bytes, store.get(cid));
        OUTCOME_TRY(cid_bytes, cid.toBytes());
        writeUvarint(output, cid_bytes.size() + bytes.size());
        output.put(cid_bytes);
        output.put(bytes);
        // TODO(turuslan): what about other types?
        if (cid.content_type == libp2p::multi::MulticodecType::DAG_CBOR) {
          try {
            CborDecodeStream s{bytes};
            visitCbor(s);
          } catch (std::system_error &e) {
            return outcome::failure(e.code());
          }
        }
      }
      return outcome::success();
    }

    void visitCbor(CborDecodeStream &s) {
      if (s.isCid()) {
        CID cid;
        s >> cid;
        auto result = visit(cid);
        if (!result) {
          outcome::raise(result.error());
        }
      } else if (s.isList()) {
        auto n = s.listLength();
        auto l = s.list();
        for (; n != 0; --n) {
          visitCbor(l);
        }
      } else if (s.isMap()) {
        for (auto &p : s.map()) {
          visitCbor(p.second);
        }
      } else {
        s.next();
      }
    }

    Ipld &store;
    Buffer &output;
    std::set<CID> visited{};
  };

  outcome::result<Buffer> makeCar(Ipld &store, const std::vector<CID> &roots) {
    Buffer output;
    OUTCOME_TRY(header_bytes,
                codec::cbor::encode(CarHeader{roots, CarHeader::V1}));
    writeUvarint(output, header_bytes.size());
    output.put(header_bytes);
    WriteVisitor visitor{store, output};
    for (auto &root : roots) {
      OUTCOME_TRY(visitor.visit(root));
    }
    return std::move(output);
  }
}  // namespace fc::storage::car
