/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "testutil/primitives/ticket_utils.hpp"

using fc::crypto::vrf::VRFProof;
using libp2p::crypto::random::BoostRandomGenerator;

TicketGenerator::TicketGenerator()
    : random_{std::make_shared<BoostRandomGenerator>()} {}

TicketGenerator::Ticket TicketGenerator::makeTicket() {
  Ticket ticket{};
  constexpr size_t size = ticket.bytes.size();
  auto bytes = random_->randomBytes(size);
  std::copy(bytes.begin(), bytes.end(), ticket.bytes.begin());

  return ticket;
}

TicketGenerator::EPostTicket TicketGenerator::makeEPostTicket() {
  auto &&int_fields = random_->randomBytes(2);
  EPostTicket ticket{.partial = {},
                     .challenge_index = int_fields[0],
                     .sector_id = int_fields[1]};
  constexpr size_t size = ticket.partial.size();
  auto &&bytes = random_->randomBytes(size);
  std::copy(bytes.begin(), bytes.end(), ticket.partial.begin());

  return ticket;
}

TicketGenerator::EPostProof TicketGenerator::makeEPostProof(
    size_t proof_size, size_t candidates_count) {
  EPostProof proof{};
  proof.proof = fc::common::Buffer(random_->randomBytes(proof_size));
  constexpr auto post_rand_size = proof.post_rand.size();
  auto bytes = random_->randomBytes(post_rand_size);
  std::copy(bytes.begin(), bytes.end(), proof.post_rand.begin());

  proof.candidates.resize(candidates_count);
  for (auto i = 0u; i < candidates_count; ++i) {
    proof.candidates.push_back(makeEPostTicket());
  }

  return proof;
}
