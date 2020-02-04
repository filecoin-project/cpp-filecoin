/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proofs/proofs.hpp"

#include <fcntl.h>
#include <filecoin-ffi/filecoin.h>
#include <boost/filesystem/fstream.hpp>
#include <common/logger.hpp>
#include <iostream>
#include "proofs/proofs_error.hpp"

namespace fc::proofs {

  using common::Blob;
  using crypto::randomness::Randomness;

  // ******************
  // TO CPP CASTED FUNCTIONS
  // ******************

  Blob<kCommitmentBytesLen> cppCommitment(const uint8_t *src) {
    Blob<kCommitmentBytesLen> result;
    for (size_t i = 0; i < kCommitmentBytesLen; i++) {
      result[i] = src[i];
    }
    return result;
  }

  std::vector<uint8_t> cppBytes(const uint8_t *src, const uint64_t size) {
    std::vector<uint8_t> result;
    for (size_t i = 0; i < size; i++) {
      result.push_back(src[i]);
    }
    return result;
  }

  Candidate cppCandidate(const FFICandidate *c_candidate) {
    Candidate candidate;
    candidate.sector_id = c_candidate->sector_id;
    candidate.sector_challenge_index = c_candidate->sector_challenge_index;
    for (size_t i = 0; i < candidate.ticket.size(); i++) {
      candidate.ticket[i] = c_candidate->ticket[i];
    }
    for (size_t i = 0; i < candidate.partial_ticket.size(); i++) {
      candidate.partial_ticket[i] = c_candidate->partial_ticket[i];
    }
    return candidate;
  }

  std::vector<Candidate> cppCandidates(const FFICandidate *candidates_ptr,
                                       size_t size) {
    std::vector<Candidate> cpp_candidates;
    if (candidates_ptr == nullptr || size == 0) {
      return cpp_candidates;
    }

    for (size_t i = 0; i < size; i++) {
      auto current_candidate = candidates_ptr + i;
      cpp_candidates.push_back(cppCandidate(current_candidate));
    }

    return cpp_candidates;
  }

  RawSealPreCommitOutput cppRawSealPreCommitOutput(
      const FFISealPreCommitOutput &c_seal_pre_commit_output) {
    RawSealPreCommitOutput cpp_seal_pre_commit_output;

    for (size_t i = 0; i < kCommitmentBytesLen; i++) {
      cpp_seal_pre_commit_output.comm_d[i] = c_seal_pre_commit_output.comm_d[i];
      cpp_seal_pre_commit_output.comm_r[i] = c_seal_pre_commit_output.comm_r[i];
    }

    return cpp_seal_pre_commit_output;
  }

  WriteWithoutAlignmentResult cppWriteWithoutAlignmentResult(
      const uint64_t total_write_unpadded, const uint8_t *comm_p) {
    WriteWithoutAlignmentResult result;

    result.total_write_unpadded = total_write_unpadded;
    result.comm_p = cppCommitment(comm_p);

    return result;
  }

  WriteWithAlignmentResult cppWriteWithAlignmentResult(
      const uint64_t left_alignment_unpadded,
      const uint64_t total_write_unpadded,
      const uint8_t *comm_p) {
    WriteWithAlignmentResult result;

    result.left_alignment_unpadded = left_alignment_unpadded;
    result.total_write_unpadded = total_write_unpadded;
    result.comm_p = cppCommitment(comm_p);

    return result;
  }

  // ******************
  // TO С CASTED FUNCTIONS
  // ******************

  FFISectorClass cSectorClass(const uint64_t sector_size,
                              const uint8_t porep_proof_partitions) {
    FFISectorClass sector_class;
    sector_class.sector_size = sector_size;
    sector_class.porep_proof_partitions = porep_proof_partitions;
    return sector_class;
  }

  FFICandidate cCandidate(const Candidate &cpp_candidate) {
    FFICandidate c_candidate;
    c_candidate.sector_id = cpp_candidate.sector_id;
    c_candidate.sector_challenge_index = cpp_candidate.sector_challenge_index;
    for (size_t i = 0; i < cpp_candidate.partial_ticket.size(); i++) {
      c_candidate.partial_ticket[i] = cpp_candidate.partial_ticket[i];
    }
    for (size_t i = 0; i < cpp_candidate.ticket.size(); i++) {
      c_candidate.ticket[i] = cpp_candidate.ticket[i];
    }
    return c_candidate;
  }

  std::vector<FFICandidate> cCandidates(
      gsl::span<const Candidate> cpp_candidates) {
    std::vector<FFICandidate> c_candidates;
    for (const auto cpp_candidate : cpp_candidates) {
      c_candidates.push_back(cCandidate(cpp_candidate));
    }
    return c_candidates;
  }

  FFIPrivateReplicaInfo cPrivateReplicaInfo(
      const PrivateReplicaInfo &cpp_private_replica_info) {
    FFIPrivateReplicaInfo c_private_replica_info;

    c_private_replica_info.sector_id = cpp_private_replica_info.sector_id;

    c_private_replica_info.cache_dir_path =
        cpp_private_replica_info.cache_dir_path.data();
    c_private_replica_info.replica_path =
        cpp_private_replica_info.sealed_sector_path.data();

    for (size_t i = 0; i < cpp_private_replica_info.comm_r.size(); i++) {
      c_private_replica_info.comm_r[i] = cpp_private_replica_info.comm_r[i];
    }

    return c_private_replica_info;
  }

  std::vector<FFIPrivateReplicaInfo> cPrivateReplicasInfo(
      gsl::span<const PrivateReplicaInfo> cpp_private_replicas_info) {
    std::vector<FFIPrivateReplicaInfo> c_private_replicas_info;
    for (long i = 0; i < cpp_private_replicas_info.size(); i++) {
      c_private_replicas_info.push_back(
          cPrivateReplicaInfo(cpp_private_replicas_info[i]));
    }
    return c_private_replicas_info;
  }

  FFIPublicPieceInfo cPublicPieceInfo(
      const PublicPieceInfo &cpp_public_piece_info) {
    FFIPublicPieceInfo c_public_piece_info;

    c_public_piece_info.num_bytes = cpp_public_piece_info.size;
    for (size_t j = 0; j < kCommitmentBytesLen; j++) {
      c_public_piece_info.comm_p[j] = cpp_public_piece_info.comm_p[j];
    }

    return c_public_piece_info;
  }

  std::vector<FFIPublicPieceInfo> cPublicPiecesInfo(
      gsl::span<const PublicPieceInfo> cpp_public_pieces_info) {
    std::vector<FFIPublicPieceInfo> c_public_pieces_info;
    for (long i = 0; i < cpp_public_pieces_info.size(); i++) {
      c_public_pieces_info.push_back(
          cPublicPieceInfo(cpp_public_pieces_info[i]));
    }
    return c_public_pieces_info;
  }

  // ******************
  // VERIFIED FUNCTIONS
  // ******************

  outcome::result<bool> verifyPoSt(const uint64_t sector_size,
                                   const SortedPublicSectorInfo &sector_info,
                                   const Randomness &randomness,
                                   const uint64_t challenge_count,
                                   gsl::span<const uint8_t> proof,
                                   gsl::span<const Candidate> winners,
                                   const Blob<32> &prover_id) {
    std::vector<std::array<uint8_t, kCommitmentBytesLen>> sorted_comrs;
    std::vector<uint64_t> sorted_sector_ids;
    for (auto sector_info_elem : sector_info.values) {
      sorted_sector_ids.push_back(sector_info_elem.sector_id);
      sorted_comrs.push_back(sector_info_elem.comm_r);
    }

    std::vector<uint8_t> flattening;
    for (size_t i = 0; i < sorted_comrs.size(); i++) {
      for (size_t j = 0; j < kCommitmentBytesLen; j++) {
        flattening.push_back(sorted_comrs[i][j]);
      }
    }

    const uint8_t(*c_randomness)[32] = &(randomness._M_elems);

    const uint8_t(*c_prover_id)[32] = &(prover_id._M_elems);

    std::vector<FFICandidate> c_winners = cCandidates(winners);

    VerifyPoStResponse *resPtr = verify_post(sector_size,
                                             c_randomness,
                                             challenge_count,
                                             sorted_sector_ids.data(),
                                             sorted_sector_ids.size(),
                                             flattening.data(),
                                             flattening.size(),
                                             proof.data(),
                                             proof.size(),
                                             c_winners.data(),
                                             c_winners.size(),
                                             c_prover_id);

    if (resPtr->status_code != 0) {
      auto logger = common::createLogger("proofs");
      logger->error("verifyPoSt: " + std::string(resPtr->error_msg));
      destroy_verify_post_response(resPtr);
      return ProofsError::UNKNOWN;
    }
    bool result = resPtr->is_valid;
    destroy_verify_post_response(resPtr);
    return result;
  }

  // ******************
  // GENERATED FUNCTIONS
  // ******************

  outcome::result<std::vector<Candidate>> generateCandidates(
      const uint64_t sector_size,
      const Blob<32> &prover_id,
      const Randomness &randomness,
      const uint64_t challenge_count,
      const SortedPrivateReplicaInfo &sorted_private_replica_info) {
    const uint8_t(*c_randomness)[32] = &(randomness._M_elems);

    const uint8_t(*c_prover_id)[32] = &(prover_id._M_elems);

    std::vector<FFIPrivateReplicaInfo> c_sorted_private_sector_info =
        cPrivateReplicasInfo(sorted_private_replica_info.values);

    auto resPtr = generate_candidates(sector_size,
                                      c_randomness,
                                      challenge_count,
                                      c_sorted_private_sector_info.data(),
                                      c_sorted_private_sector_info.size(),
                                      c_prover_id);
    if (resPtr->status_code != 0) {
      auto logger = common::createLogger("proofs");
      logger->error("generateCandidates: " + std::string(resPtr->error_msg));
      destroy_generate_candidates_response(resPtr);
      return ProofsError::UNKNOWN;
    }

    auto result = cppCandidates(resPtr->candidates_ptr, resPtr->candidates_len);
    destroy_generate_candidates_response(resPtr);
    return result;
  }

  outcome::result<std::vector<uint8_t>> generatePoSt(
      const uint64_t sectorSize,
      const Blob<32> &prover_id,
      const SortedPrivateReplicaInfo &private_replica_info,
      const Randomness &randomness,
      gsl::span<const Candidate> winners) {
    std::vector<FFICandidate> c_winners = cCandidates(winners);

    const uint8_t(*c_randomness)[32] = &(randomness._M_elems);

    const uint8_t(*c_prover_id)[32] = &(prover_id._M_elems);

    std::vector<FFIPrivateReplicaInfo> c_sorted_private_sector_info =
        cPrivateReplicasInfo(private_replica_info.values);

    auto resPtr = generate_post(sectorSize,
                                c_randomness,
                                c_sorted_private_sector_info.data(),
                                c_sorted_private_sector_info.size(),
                                c_winners.data(),
                                c_winners.size(),
                                c_prover_id);

    if (resPtr->status_code != 0) {
      auto logger = common::createLogger("proofs");
      logger->error("generatePoSt: " + std::string(resPtr->error_msg));
      destroy_generate_post_response(resPtr);
      return ProofsError::UNKNOWN;
    }

    auto result =
        cppBytes(resPtr->flattened_proofs_ptr, resPtr->flattened_proofs_len);
    destroy_generate_post_response(resPtr);
    return result;
  }

  outcome::result<Blob<kCommitmentBytesLen>> generatePieceCommitmentFromFile(
      const std::string &piece_file_path, const uint64_t piece_size) {
    int fd = open(piece_file_path.c_str(), O_RDWR);

    auto res_ptr = generate_piece_commitment(fd, piece_size);

    if (res_ptr->status_code != 0) {
      destroy_generate_piece_commitment_response(res_ptr);
      return ProofsError::UNKNOWN;
    }

    auto result = cppCommitment(res_ptr->comm_p);
    destroy_generate_piece_commitment_response(res_ptr);
    return result;
  }

  outcome::result<Blob<kCommitmentBytesLen>> generateDataCommitment(
      const uint64_t sector_size, gsl::span<const PublicPieceInfo> pieces) {
    std::vector<FFIPublicPieceInfo> c_pieces = cPublicPiecesInfo(pieces);

    auto res_ptr =
        generate_data_commitment(sector_size, c_pieces.data(), c_pieces.size());

    if (res_ptr->status_code != 0) {
      destroy_generate_data_commitment_response(res_ptr);
      return ProofsError::UNKNOWN;
    }

    auto result = cppCommitment(res_ptr->comm_d);
    destroy_generate_data_commitment_response(res_ptr);
    return result;
  }

  outcome::result<WriteWithoutAlignmentResult> writeWithoutAlignment(
      const std::string &piece_file_path,
      const uint64_t piece_bytes,
      const std::string &staged_sector_file_path) {
    int piece_fd = open(piece_file_path.c_str(), O_RDWR);
    int staged_sector_fd = open(staged_sector_file_path.c_str(), O_RDWR);

    auto resPtr =
        write_without_alignment(piece_fd, piece_bytes, staged_sector_fd);

    if (resPtr->status_code != 0) {
      auto logger = common::createLogger("proofs");
      logger->error("writeWithoutAlignment: " + std::string(resPtr->error_msg));
      destroy_write_without_alignment_response(resPtr);
      return ProofsError::UNKNOWN;
    }

    auto result = cppWriteWithoutAlignmentResult(resPtr->total_write_unpadded,
                                                 resPtr->comm_p);
    destroy_write_without_alignment_response(resPtr);
    return result;
  }

  outcome::result<WriteWithAlignmentResult> writeWithAlignment(
      const std::string &piece_file_path,
      const uint64_t piece_bytes,
      const std::string &staged_sector_file_path,
      gsl::span<const uint64_t> existing_piece_sizes) {
    int piece_fd = open(piece_file_path.c_str(), O_RDWR);
    int staged_sector_fd = open(staged_sector_file_path.c_str(), O_RDWR);

    auto resPtr = write_with_alignment(piece_fd,
                                       piece_bytes,
                                       staged_sector_fd,
                                       existing_piece_sizes.data(),
                                       existing_piece_sizes.size());

    if (resPtr->status_code != 0) {
      auto logger = common::createLogger("proofs");
      logger->error("writeWithAlignment: " + std::string(resPtr->error_msg));
      destroy_write_with_alignment_response(resPtr);
      return ProofsError::UNKNOWN;
    }

    auto result = cppWriteWithAlignmentResult(resPtr->left_alignment_unpadded,
                                              resPtr->total_write_unpadded,
                                              resPtr->comm_p);
    destroy_write_with_alignment_response(resPtr);
    return result;
  }

  outcome::result<RawSealPreCommitOutput> sealPreCommit(
      const uint64_t sector_size,
      const uint8_t porep_proof_partitions,
      const std::string &cache_dir_path,
      const std::string &staged_sector_path,
      const std::string &sealed_sector_path,
      const uint64_t sector_id,
      const Blob<32> &prover_id,
      const Blob<32> &ticket,
      gsl::span<const PublicPieceInfo> pieces) {
    const uint8_t(*c_prover_id)[32] = &(prover_id._M_elems);

    const uint8_t(*c_ticket)[32] = &(ticket._M_elems);

    std::vector<FFIPublicPieceInfo> c_pieces = cPublicPiecesInfo(pieces);

    auto resPtr =
        seal_pre_commit(cSectorClass(sector_size, porep_proof_partitions),
                        cache_dir_path.c_str(),
                        staged_sector_path.c_str(),
                        sealed_sector_path.c_str(),
                        sector_id,
                        c_prover_id,
                        c_ticket,
                        c_pieces.data(),
                        c_pieces.size());
    if (resPtr->status_code != 0) {
      auto logger = common::createLogger("proofs");
      logger->error("sealPreCommit: " + std::string(resPtr->error_msg));
      destroy_seal_pre_commit_response(resPtr);
      return ProofsError::UNKNOWN;
    }
    auto result = cppRawSealPreCommitOutput(resPtr->seal_pre_commit_output);
    destroy_seal_pre_commit_response(resPtr);
    return result;
  }

  SortedPrivateReplicaInfo newSortedPrivateReplicaInfo(
      gsl::span<const PrivateReplicaInfo> replica_info) {
    SortedPrivateReplicaInfo sorted_sector_info;

    for (const auto &elem : replica_info) {
      sorted_sector_info.values.push_back(elem);
    }
    std::sort(sorted_sector_info.values.begin(),
              sorted_sector_info.values.end(),
              [](const PrivateReplicaInfo &lhs, const PrivateReplicaInfo &rhs) {
                return std::memcmp(lhs.comm_r.data(),
                                   rhs.comm_r.data(),
                                   lhs.comm_r.size())
                       < 0;
              });

    return sorted_sector_info;
  }

  SortedPublicSectorInfo newSortedPublicSectorInfo(
      gsl::span<const PublicSectorInfo> sector_info) {
    SortedPublicSectorInfo sorted_sector_info;

    for (const auto &elem : sector_info) {
      sorted_sector_info.values.push_back(elem);
    }
    std::sort(sorted_sector_info.values.begin(),
              sorted_sector_info.values.end(),
              [](const PublicSectorInfo &lhs, const PublicSectorInfo &rhs) {
                return std::memcmp(lhs.comm_r.data(),
                                   rhs.comm_r.data(),
                                   lhs.comm_r.size())
                       < 0;
              });

    return sorted_sector_info;
  }
}  // namespace fc::proofs
