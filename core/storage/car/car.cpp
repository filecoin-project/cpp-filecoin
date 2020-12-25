/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include "codec/uvarint.hpp"
#include "common/logger.hpp"
#include "storage/car/car.hpp"
#include "storage/ipld/traverser.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::car, CarError, e) {
  using E = fc::storage::car::CarError;
  switch (e) {
    case E::kDecodeError:
      return "Decode error";
    case E::kReadFileError:
      return "Read car file error";
  }
}

namespace fc::storage::car {
  using ipld::kAllSelector;
  using ipld::traverser::Traverser;

  outcome::result<std::vector<CID>> loadCar(Ipld &store, Input input) {
    OUTCOME_TRY(header_bytes,
                codec::uvarint::readBytes<CarError::kDecodeError,
                                          CarError::kDecodeError>(input));
    OUTCOME_TRY(header, codec::cbor::decode<CarHeader>(header_bytes));
    while (!input.empty()) {
      OUTCOME_TRY(node,
                  codec::uvarint::readBytes<CarError::kDecodeError,
                                            CarError::kDecodeError>(input));
      OUTCOME_TRY(cid, CID::read(node));
      OUTCOME_TRY(store.set(cid, common::Buffer{node}));
    }
    return std::move(header.roots);
  }

  outcome::result<std::vector<CID>> loadCar(storage::PersistentBufferMap &store,
                                            const std::string &file_name) {
    namespace bip = boost::interprocess;

    auto log = common::createLogger("load_car");

    gsl::span<const uint8_t> input;

    try {
      bip::file_mapping file(file_name.c_str(), bip::read_only);
      bip::mapped_region region(file, bip::read_only);
      void *addr = region.get_address();
      std::size_t size = region.get_size();
      input =
          gsl::span<const uint8_t>(static_cast<const uint8_t *>(addr), size);
      log->info("reading file {} of size {}", file_name, size);

      size_t reported_percent = 0;
      const size_t total_bytes = input.size();
      size_t objects_count = 0;
      auto progress = [&](size_t current) {
        auto x = current * 100 / total_bytes;
        if (x > reported_percent) {
          reported_percent = x;
          log->info("{}%, {} objects stored", reported_percent, objects_count);
        }
      };

      OUTCOME_TRY(header_bytes,
                  codec::uvarint::readBytes<CarError::kDecodeError,
                                            CarError::kDecodeError>(input));
      OUTCOME_TRY(header, codec::cbor::decode<CarHeader>(header_bytes));
      log->info("read header, {} roots", header.roots.size());
      if (header.roots.size() < 100) {
        for (const auto &r : header.roots) {
          log->info(r.toString().value());
        }
      }

      static constexpr size_t kBatchSize = 100000;
      size_t current_batch_size = 0;
      auto batch = store.batch();

      while (!input.empty()) {
        OUTCOME_TRY(node,
                    codec::uvarint::readBytes<CarError::kDecodeError,
                                              CarError::kDecodeError>(input));
        auto cid_bytes = node;
        OUTCOME_TRY(cid, CID::read(node));
        std::ignore = cid;
        cid_bytes = cid_bytes.first(cid_bytes.size() - node.size());

        OUTCOME_TRY(
            batch->put(common::Buffer{cid_bytes}, common::Buffer{node}));

        if (++current_batch_size >= kBatchSize) {
          OUTCOME_TRY(batch->commit());
          batch->clear();
          current_batch_size = 0;
        }

        OUTCOME_TRY(batch->commit());
        batch->clear();

        ++objects_count;
        progress(total_bytes - input.size());
      }

      return std::move(header.roots);

    } catch (const std::exception &e) {
      log->error("exception while reading {}: {}", file_name, e.what());
      return CarError::kReadFileError;
    } catch (...) {
      log->error("unknown exception while reading {}", file_name);
      return CarError::kReadFileError;
    }
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
      Traverser traverser{store, root, kAllSelector};
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
      Traverser traverser{store, dag.first, dag.second};
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
}  // namespace fc::storage::car
