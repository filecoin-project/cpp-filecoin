/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_PIECE_DATA_HPP
#define CPP_FILECOIN_PIECE_DATA_HPP

#include <string>

namespace fc::primitives::piece {

  const int kUnopenedFileDescriptor = -1;

  class PieceData {
   public:
    explicit PieceData(const std::string &path_to_file);
    explicit PieceData(int &pipe_fd);

    PieceData(PieceData &&other) noexcept;

    ~PieceData();

    int getFd() const;

    bool isOpened() const;

   private:
    int fd_;
  };

}  // namespace fc::primitives::piece

#endif  // CPP_FILECOIN_PIECE_DATA_HPP
