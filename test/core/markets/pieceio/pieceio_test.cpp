/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/pieceio/pieceio_impl.hpp"

#include <gmock/gmock.h>

#include <boost/filesystem.hpp>
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "storage/unixfs/unixfs.hpp"
#include "testutil/outcome.hpp"
#include "testutil/read_file.hpp"
#include "testutil/resources/resources.hpp"

using fc::markets::pieceio::PieceIOImpl;
using fc::primitives::piece::UnpaddedPieceSize;
using fc::storage::ipfs::InMemoryDatastore;
using fc::storage::ipfs::IpfsDatastore;

/**
 * Interop test with go-fil-markets/storagemarket/integration_test.go
 * @given PAYLOAD_FILE with some data, commitment cid of cmmitment generated in
 * go integration test
 * @when make commitment from root payload cid
 * @then cmmitment and padded size are equal to generated in go
 * TODO(artyom-yurin): [FIL-240]
 * @note Disabled because .car file contains old CIDs
 */
TEST(PieceIO, DISABLED_generatePieceCommitment) {
  std::shared_ptr<IpfsDatastore> ipld = std::make_shared<InMemoryDatastore>();
  auto input = readFile(PAYLOAD_FILE);
  EXPECT_OUTCOME_TRUE(payload_cid, fc::storage::unixfs::wrapFile(*ipld, input));

  PieceIOImpl piece_io{ipld, boost::filesystem::temp_directory_path().string()};
  EXPECT_OUTCOME_TRUE(
      res,
      piece_io.generatePieceCommitment(
          fc::primitives::sector::RegisteredProof::StackedDRG2KiBWindowPoSt,
          payload_cid,
          {}));

  // padded size from go-fil-markets integration test
  UnpaddedPieceSize expected_padded_size{32512};
  EXPECT_EQ(expected_padded_size, res.second);

  // commitment cid from go-fil-markets integration test
  std::string commitment_cid_expected{
      "bafk4chza4qfcf6rqteuaudxm3m2liuv2ajcixckjjfcb5vgdbay4pc5jluja"};
  EXPECT_OUTCOME_TRUE(commitment_cid, res.first.toString());
  EXPECT_EQ(commitment_cid_expected, commitment_cid);
}
