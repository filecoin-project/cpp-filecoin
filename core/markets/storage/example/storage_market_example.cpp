/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fstream>
#include <iostream>
#include "common/buffer.hpp"
#include "common/span.hpp"
#include "common/todo_error.hpp"
#include "markets/pieceio/pieceio_impl.hpp"
#include "markets/storage/deal_protocol.hpp"
#include "markets/storage/example/resources.hpp"
#include "primitives/sector/sector.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"

namespace fc::markets::storage::example {

  using common::Buffer;
  using fc::storage::ipfs::InMemoryDatastore;
  using fc::storage::ipfs::IpfsDatastore;
  using pieceio::PieceIO;
  using pieceio::PieceIOImpl;
  using primitives::sector::RegisteredProof;

  static const RegisteredProof registered_proof{
      RegisteredProof::StackedDRG32GiBSeal};

  /**
   * Payload CID from Go
   */
  static const std::string kPayloadCid =
      "QmXFz92Uc9gCyAVGKkCzD84HEiR9fmrFzPSrvUypaN2Yzx";

  /**
   * Reads file into Buffer
   * @param path to file
   * @return file content as Buffer
   */
  outcome::result<Buffer> readFile(const std::string &path) {
    std::ifstream file{path, std::ios::binary | std::ios::ate};
    if (!file.good()) {
      std::cerr << "Cannot open file: " << path;
      return TodoError::ERROR;
    }
    Buffer buffer;
    buffer.resize(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(fc::common::span::string(buffer).data(), buffer.size());
    return buffer;
  }

  outcome::result<DataRef> makeDataRef() {
    std::shared_ptr<IpfsDatastore> ipfs_datastore =
        std::make_shared<InMemoryDatastore>();
    std::shared_ptr<PieceIO> piece_io =
        std::make_shared<PieceIOImpl>(ipfs_datastore);

    OUTCOME_TRY(root_cid, CID::fromString(kPayloadCid));
    OUTCOME_TRY(data, readFile(CAR_FROM_PAYLOAD_FILE));
    OUTCOME_TRY(piece_commitment,
                piece_io->generatePieceCommitment(registered_proof, data));
    return DataRef{.transfer_type = kTransferTypeManual,
                   .root = root_cid,
                   .piece_cid = piece_commitment.first,
                   .piece_size = piece_commitment.second};
  }

}  // namespace fc::markets::storage::example

int main() {
  auto car_data = fc::markets::storage::example::makeDataRef().value();
}
