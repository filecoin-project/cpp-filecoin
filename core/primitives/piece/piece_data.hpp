/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_PIECE_DATA_HPP
#define CPP_FILECOIN_PIECE_DATA_HPP

#include <string>

namespace fc::primitives::piece {

  class PieceData {
   public:
    explicit PieceData(const std::string &path_to_file);

    ~PieceData();

    int getFd() const;

   private:
    int fd_;
  };

}  // namespace fc::primitives::piece

#endif  // CPP_FILECOIN_PIECE_DATA_HPP
