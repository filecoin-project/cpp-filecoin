/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "testutil/primitives/ticket/printer.hpp"

std::string print(const fc::primitives::ticket::Ticket &t) {
  std::stringstream s;
  s << "Ticket {\n"
    << "\tbytes = " << fc::common::hex_lower(t.bytes) << "\n"
    << "}\n";

  return s.str();
}

std::string print(const fc::primitives::ticket::EPostTicket &t) {
  std::stringstream s;
  s << "EPostTicket {\n"
    << "\tpartial=" << fc::common::hex_lower(t.partial) << "\n"
    << "\tsector_id = " << t.sector_id << "\n"
    << "\tchallenge_index = " << t.challenge_index << "\n"
    << "}\n";

  return s.str();
}

std::string print(const fc::primitives::ticket::EPostProof &p) {
  std::stringstream s;
  s << "EPostProof {\n"
    << "\tproof = " << fc::common::hex_lower(p.proof) << "\n"
    << "\tpost_rand = " << fc::common::hex_lower(p.post_rand) << "\n"
    << "\tcandidates {\n\t\t";
  for (auto &c : p.candidates) {
    auto &&str = print(c);
    boost::replace_all(str, "\n", "\n\t\t");
    s << str;
  }
  s << "\b}\n"
    << "}\n";

  return s.str();
}
