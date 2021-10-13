/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/pieceio/pieceio_impl.hpp"

#include <gmock/gmock.h>

#include <boost/filesystem.hpp>
#include "testutil/outcome.hpp"
#include "testutil/resources/resources.hpp"

using fc::markets::pieceio::PieceIOImpl;
using fc::primitives::piece::UnpaddedPieceSize;

/**
 * Interop test with go-fil-markets/storagemarket/integration_test.go
 * @given PAYLOAD_FILE with some data, commitment cid of commitment generated in
 * go integration test
 * @when make commitment from root payload cid
 * @then commitment and padded size are equal to generated in go
 */
TEST(PieceIO, generatePieceCommitment) {
  PieceIOImpl piece_io{boost::filesystem::temp_directory_path()};
  EXPECT_OUTCOME_TRUE(
      res,
      piece_io.generatePieceCommitment(
          fc::primitives::sector::RegisteredSealProof::kStackedDrg2KiBV1,
          PAYLOAD_FILE));

  // padded size from go-fil-markets integration test
  UnpaddedPieceSize expected_padded_size{32512};
  EXPECT_EQ(expected_padded_size, res.second);

  // commitment cid from go-fil-markets integration test
  std::string commitment_cid_expected{
      "baga6ea4seaqgycs5xk6sa4fh6ezioasumkatcdt4uae2swobyjkzmx3zloi3ogq"};
  EXPECT_OUTCOME_TRUE(commitment_cid, res.first.toString());
  EXPECT_EQ(commitment_cid_expected, commitment_cid);
}
