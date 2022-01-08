/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <fcntl.h>
#include <string>

namespace fc::primitives::piece {

  const int kUnopenedFileDescriptor = -1;

  class PieceData {
   public:
    explicit PieceData(const std::string &path_to_file, int flags = O_RDWR);
    explicit PieceData(int &pipe_fd);
    PieceData();

    PieceData(const PieceData &) = delete;
    PieceData &operator=(const PieceData &) = delete;
    PieceData(PieceData &&other) noexcept;
    PieceData &operator=(PieceData &&other) noexcept;

    ~PieceData();

    int getFd() const;

    bool isOpened() const;

    bool isNullData() const;

   private:
    int fd_;
    bool is_null_data_;
  };

}  // namespace fc::primitives::piece
