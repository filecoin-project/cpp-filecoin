//
// Created by soramitsu on 26.02.2020.
//

#ifndef CPP_FILECOIN_PIECE_ERROR_HPP
#define CPP_FILECOIN_PIECE_ERROR_HPP

#include "common/outcome.hpp"

namespace fc::primitives::piece {

  /**
   * @brief Pieces returns these types of errors
   */
  enum class PieceError {
    LESS_THAT_MINIMUM_SIZE = 1,
    LESS_THAT_MINIMUM_PADDED_SIZE,
    INVALID_UNPADDED_SIZE,
    INVALID_PADDED_SIZE,
  };

}  // namespace fc::primitives::piece

OUTCOME_HPP_DECLARE_ERROR(fc::primitives::piece, PieceError);

#endif  // CPP_FILECOIN_PIECE_ERROR_HPP
