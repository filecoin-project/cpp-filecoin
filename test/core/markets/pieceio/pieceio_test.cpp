/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/pieceio/pieceio_impl.hpp"

#include <gmock/gmock.h>

#include "storage/face/persistent_map.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "storage/piece/impl/piece_storage_impl.hpp"
#include "storage/unixfs/unixfs.hpp"
#include "testutil/outcome.hpp"
#include "testutil/read_file.hpp"
#include "testutil/resources/resources.hpp"

using fc::markets::pieceio::PieceIOImpl;
using fc::primitives::piece::UnpaddedPieceSize;
using fc::storage::ipfs::InMemoryDatastore;
using fc::storage::ipfs::IpfsDatastore;
using fc::storage::piece::PieceStorageImpl;
using fc::common::Buffer;
using PersistentMap = fc::storage::face::PersistentMap<Buffer, Buffer>;

/**
 * Interop test with go-fil-markets/storagemarket/integration_test.go
 * @given PAYLOAD_FILE with some data, commitment cid of cmmitment generated in
 * go integration test
 * @when make commitment from root payload cid
 * @then cmmitment and padded size are equal to generated in go
 */
TEST(PieceIO, generatePieceCommitment) {
  std::shared_ptr<IpfsDatastore> ipld = std::make_shared<InMemoryDatastore>();
  auto storage_backend = std::make_shared<PersistentMap>();
  auto piece_storage = std::make_shared<PieceStorageImpl>(storage_backend);
  auto input = readFile(PAYLOAD_FILE);
  EXPECT_OUTCOME_TRUE(payload_cid, fc::storage::unixfs::wrapFile(*ipld, input));

  PieceIOImpl piece_io{ipld, piece_storage};
  EXPECT_OUTCOME_TRUE(
      res,
      piece_io.generatePieceCommitment(
          fc::primitives::sector::RegisteredProof::StackedDRG2KiBWindowPoSt,
          payload_cid,
          {}));

  // padded size from fo-gil-markets integration test
  UnpaddedPieceSize expected_padded_size{32512};
  EXPECT_EQ(expected_padded_size, res.second);

  // commitment cid from fo-gil-markets integration test
  std::string commitment_cid_expected{
      "bafk4chza4qfcf6rqteuaudxm3m2liuv2ajcixckjjfcb5vgdbay4pc5jluja"};
  EXPECT_OUTCOME_TRUE(commitment_cid, res.first.toString());
  EXPECT_EQ(commitment_cid_expected, commitment_cid);
}
