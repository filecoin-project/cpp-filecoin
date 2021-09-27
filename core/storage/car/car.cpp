/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/car/car.hpp"

#include <fstream>
#include <libp2p/multi/uvarint.hpp>

#include "codec/uvarint.hpp"
#include "common/error_text.hpp"
#include "common/file.hpp"
#include "common/span.hpp"
#include "storage/ipld/traverser.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::car, CarError, e) {
  using E = fc::storage::car::CarError;
  switch (e) {
    case E::kDecodeError:
      return "Decode error";
    case E::kCannotOpenFileError:
      return "Cannot open file";
  }
}

namespace fc::storage::car {
  using ipld::kAllSelector;
  using ipld::traverser::Traverser;

  outcome::result<CarReader> CarReader::make(BytesIn file) {
    CarReader reader;
    reader.file = file;
    auto input{file};
    BytesIn header_bytes;
    if (!codec::uvarint::readBytes(header_bytes, input)) {
      return CarError::kDecodeError;
    }
    OUTCOME_TRY(header, codec::cbor::decode<CarHeader>(header_bytes));
    reader.position = input.data() - reader.file.data();
    reader.roots = std::move(header.roots);
    return reader;
  }

  bool CarReader::end() const {
    return (ssize_t)position >= file.size() || file[position] == 0;
  }

  outcome::result<CarReader::Item> CarReader::next() {
    assert(!end());
    auto input{file.subspan(position)};
    BytesIn node;
    if (!codec::uvarint::readBytes(node, input)) {
      return CarError::kDecodeError;
    }
    OUTCOME_TRY(cid, CID::read(node));
    position = input.data() - file.data();
    ++objects;
    return std::make_pair(std::move(cid), node);
  }

  outcome::result<std::vector<CID>> readHeader(const std::string &car_path) {
    std::ifstream car_file{car_path, std::ios::binary};
    Buffer buffer;
    if (codec::uvarint::readBytes(car_file, buffer)) {
      OUTCOME_TRY(header, codec::cbor::decode<CarHeader>(buffer));
      return header.roots;
    }
    return ERROR_TEXT("readHeader: read car header failed");
  }

  outcome::result<std::vector<CID>> loadCar(Ipld &store, Input input) {
    OUTCOME_TRY(reader, CarReader::make(input));
    while (!reader.end()) {
      OUTCOME_TRY(item, reader.next());
      OUTCOME_TRY(store.set(item.first, common::Buffer{item.second}));
    }
    return std::move(reader.roots);
  }

  outcome::result<std::vector<CID>> loadCar(Ipld &store,
                                            const std::string &car_path) {
    OUTCOME_TRY(file, common::mapFile(car_path));
    return loadCar(store, file.second);
  }

  outcome::result<std::vector<CID>> loadCar(
      Ipld &store, const boost::filesystem::path &car_path) {
    return loadCar(store, car_path.string());
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
                                  const std::vector<CID> &cids) {
    Buffer output;
    writeHeader(output, roots);
    for (auto &cid : cids) {
      OUTCOME_TRY(writeItem(output, store, cid));
    }
    return std::move(output);
  }

  outcome::result<Buffer> makeCar(Ipld &store, const std::vector<CID> &roots) {
    std::set<CID> cids;
    for (auto &root : roots) {
      Traverser traverser{store, root, kAllSelector, true};
      OUTCOME_TRY(visited, traverser.traverseAll());
      cids.insert(visited.begin(), visited.end());
    }
    return makeCar(store, roots, {cids.begin(), cids.end()});
  }

  outcome::result<Buffer> makeSelectiveCar(
      Ipld &store, const std::vector<std::pair<CID, Selector>> &dags) {
    std::vector<CID> roots;
    std::vector<CID> cid_order;
    std::set<CID> cids;
    for (auto &dag : dags) {
      Traverser traverser{store, dag.first, dag.second, true};
      OUTCOME_TRY(visited, traverser.traverseAll());
      roots.push_back(dag.first);
      for (auto &cid : visited) {
        if (cids.insert(cid).second) {
          cid_order.push_back(cid);
        }
      }
    }
    return makeCar(store, roots, cid_order);
  }

  void writeUvarint(std::ostream &output, uint64_t value) {
    output << common::span::bytestr(libp2p::multi::UVarint{value}.toBytes());
  }

  void writeHeader(std::ostream &output, const std::vector<CID> &roots) {
    OUTCOME_EXCEPT(bytes, codec::cbor::encode(CarHeader{roots, CarHeader::V1}));
    writeUvarint(output, bytes.size());
    output << common::span::bytestr(bytes);
  }

  void writeItem(std::ostream &output, const CID &cid, Input bytes) {
    OUTCOME_EXCEPT(cid_bytes, cid.toBytes());
    writeUvarint(output, cid_bytes.size() + bytes.size());
    output << common::span::bytestr(cid_bytes);
    output << common::span::bytestr(bytes);
  }

  outcome::result<void> writeItem(std::ostream &output,
                                  Ipld &store,
                                  const CID &cid) {
    OUTCOME_TRY(bytes, store.get(cid));
    writeItem(output, cid, bytes);
    return outcome::success();
  }

  outcome::result<void> makeCar(std::ostream &output,
                                Ipld &store,
                                const std::vector<CID> &roots,
                                const std::vector<CID> &cids) {
    writeHeader(output, roots);
    for (auto &cid : cids) {
      OUTCOME_TRY(writeItem(output, store, cid));
    }

    return outcome::success();
  }

  outcome::result<void> makeSelectiveCar(
      Ipld &store,
      const std::vector<std::pair<CID, Selector>> &dags,
      const std::string &output_path) {
    std::ofstream output(output_path,
                         std::ios_base::out | std::ios_base::binary);
    if (!output.good()) {
      return CarError::kCannotOpenFileError;
    }
    std::vector<CID> roots;
    std::vector<CID> cid_order;
    std::set<CID> cids;
    for (auto &dag : dags) {
      Traverser traverser{store, dag.first, dag.second, true};
      OUTCOME_TRY(visited, traverser.traverseAll());
      roots.push_back(dag.first);
      for (auto &cid : visited) {
        if (cids.insert(cid).second) {
          cid_order.push_back(cid);
        }
      }
    }
    return makeCar(output, store, roots, cid_order);
  }
}  // namespace fc::storage::car
