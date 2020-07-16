/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/common/literals.hpp>
#include "common/buffer.hpp"
#include "storage/piece/piece_storage.hpp"
#include "testutil/literals.hpp"

namespace fc::markets::retrieval::test {
  /**
   * @brief Payload - one of the blocks, of which consists Piece
   */
  struct SamplePayload {
    CID cid;
    common::Buffer content;
    ::fc::storage::piece::PayloadLocation location;
  };

  /**
   * @brief Piece - data unit, which is stored in Sectors
   */
  struct SamplePiece {
    ::fc::storage::piece::PieceInfo info;
    std::vector<SamplePayload> payloads;
  };

  namespace data {
    using libp2p::common::operator""_unhex;

    SamplePiece green_piece{
        .info = {.piece_cid = "12209139839e65fabea9efd230898ad8b574509147e48d7c"
                              "1e87a33d6da70fd2efae"_cid,
                 .deals = {{.deal_id = 18,
                            .sector_id = 4,
                            .offset = 128,
                            .length = 64}}},
        .payloads = {
            {.cid =
                 "12209139839e65fabea9efd230898ad8b574509147e48d7c1e87a33d6da70fd2efbf"_cid,
             .content = common::Buffer{"fafece"_unhex},
             .location = {.relative_offset = 16, .block_size = 4}}}};

  }  // namespace data
}  // namespace fc::markets::retrieval::test
