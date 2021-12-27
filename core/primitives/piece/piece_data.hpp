/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <fcntl.h>
#include <array>
#include <string>

namespace fc::primitives::piece {

  const int kUnopenedFileDescriptor = -1;

  class PieceData {
   public:
    explicit PieceData(const std::string &path_to_file, int flags = O_RDWR);
    explicit PieceData(int &pipe_fd);

    PieceData(const PieceData &) = delete;
    PieceData &operator=(const PieceData &) = delete;
    PieceData(PieceData &&other) noexcept;
    PieceData &operator=(PieceData &&other) noexcept;

    ~PieceData();

    int getFd() const;

    bool isOpened() const;

    [[nodiscard]] int release();

   private:
    int fd_;
  };

  class ReaderType {
   public:
    enum Type : uint64_t {
      pushStreamReader = 0,
      nullReader = 1,
    } reader_type;

    ReaderType(Type type) : reader_type(std::move(type)){};

    ReaderType(std::string string_type) {
      if (string_type == "push") {
        reader_type = pushStreamReader;
      } else {
        reader_type = nullReader;
      }
    }

    static const std::array<std::string, 2> types;

    const std::string toString() const;
  };

  class MetaPieceData {
   public:
    MetaPieceData(std::string uuid, ReaderType::Type type)
        : uuid(std::move(uuid)), type(ReaderType(type)){};

    std::string uuid;
    ReaderType type;
  };

}  // namespace fc::primitives::piece
