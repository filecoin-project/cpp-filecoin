/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <fstream>
#include "primitives/big_int.hpp"

namespace fc {
  using primitives::BigInt;

  inline void parseFile(const std::string &path,
                        std::function<void(std::stringstream &)> f) {
    std::ifstream in(path);
    std::vector<std::pair<BigInt, BigInt>> res;

    std::string line;
    while (!in.eof()) {
      std::getline(in, line);
      if (line.empty()) continue;
      std::stringstream ss(line);
      f(ss);
    }
  }

  /**
   * Parses CVS file as x,y pairs on each line
   * @param path to the file
   * @return (x, y) vector
   */
  inline std::vector<std::pair<BigInt, BigInt>> parseCsvPair(
      const std::string &path) {
    std::vector<std::pair<BigInt, BigInt>> res;
    auto parsePair = [&res](std::stringstream &ss) {
      std::string x;
      std::getline(ss, x, ',');

      std::string y;
      std::getline(ss, y);

      res.emplace_back(std::make_pair(BigInt(x), BigInt(y)));
    };

    parseFile(path, parsePair);

    return res;
  }

  /**
   * Parses CVS file as x,y,z pairs on each line
   * @param path to the file
   * @return (x, y, z) vector
   */
  inline std::vector<std::tuple<BigInt, BigInt, BigInt>> parseCsvTriples(
      const std::string &path) {
    std::vector<std::tuple<BigInt, BigInt, BigInt>> res;
    auto parsePair = [&res](std::stringstream &ss) {
      std::string x;
      std::getline(ss, x, ',');

      std::string y;
      std::getline(ss, y, ',');

      std::string z;
      std::getline(ss, z);

      res.push_back(std::make_tuple(BigInt(x), BigInt(y), BigInt(z)));
    };

    parseFile(path, parsePair);

    return res;
  }

}  // namespace fc
