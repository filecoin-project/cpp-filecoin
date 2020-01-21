//
// Created by ortyomka on 17.01.2020.
//

#ifndef CPP_FILECOIN_UTIL_HPP
#define CPP_FILECOIN_UTIL_HPP

namespace fc::vm::actor {
  //TODO: temp structure, it will be removed after it will be implemented
  class SectorStorageWeightDesc {};

  static inline bool operator==(const SectorStorageWeightDesc &lhs,
                                const SectorStorageWeightDesc &rhs) {
    return true;
  }
}  // namespace fc::vm::actor

#endif  // CPP_FILECOIN_UTIL_HPP
