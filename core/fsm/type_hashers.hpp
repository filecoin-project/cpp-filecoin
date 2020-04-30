/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_FSM_TYPE_HASHERS_HPP
#define CPP_FILECOIN_CORE_FSM_TYPE_HASHERS_HPP

#include <boost/container_hash/hash.hpp>

namespace fc::common {

  /// Enables use of enum class values as keys in std::unordered_map
  struct EnumClassHash {
    template <typename T>
    std::size_t operator()(T t) const {
      return static_cast<std::size_t>(t);
    }
  };

  template <typename T, typename... Rest>
  void HashCombine(std::size_t &seed, const T &v, const Rest &... rest) {
    boost::hash_combine(seed, v);
    (HashCombine(seed, rest), ...);
  }

}  // namespace fc::common

/**
 * Enable any user type to be used as a key in std::unordered_map
 *
 * Usage example:
 *
 * struct MyContainer{
 *  int counter;
 *  std::string name;
 * }
 *
 * MAKE_HASHABLE(MyContainer, t.counter, t.name);
 */
#define MAKE_HASHABLE(Type, ...)                    \
  namespace std {                                   \
    template <>                                     \
    struct hash<Type> {                             \
      std::size_t operator()(const Type &t) const { \
        std::size_t ret = 0;                        \
        fc::common::HashCombine(ret, __VA_ARGS__);  \
        return ret;                                 \
      }                                             \
    };                                              \
  }

#endif  // CPP_FILECOIN_CORE_FSM_TYPE_HASHERS_HPP
