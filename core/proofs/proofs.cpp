/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proofs/proofs.hpp"

#include <fcntl.h>
#include <filecoin-ffi/filecoin.h>
#include "boost/filesystem/fstream.hpp"
#include "proofs/proofs_error.hpp"

namespace fc::proofs {

  common::Logger Proofs::logger = common::createLogger("proofs");

  using common::Blob;
  using crypto::randomness::Randomness;

  template <typename T, typename D>
  auto make_unique(T *ptr, D deleter) {
    return std::unique_ptr<T, D>(ptr, deleter);
  }

  // ******************
  // TO CPP CASTED FUNCTIONS
  // ******************

  Comm cppCommitment(const gsl::span<uint8_t, 32> &bytes) {
    Comm result;

    std::copy(bytes.begin(), bytes.end(), result.begin());

    return result;
  }

  Candidate cppCandidate(const FFICandidate &c_candidate) {
    Candidate candidate;
    candidate.sector_id = c_candidate.sector_id;
    candidate.sector_challenge_index = c_candidate.sector_challenge_index;
    std::copy(c_candidate.ticket,
              c_candidate.ticket + candidate.ticket.size(),
              candidate.ticket.begin());
    std::copy(c_candidate.partial_ticket,
              c_candidate.partial_ticket + candidate.partial_ticket.size(),
              candidate.partial_ticket.begin());
    return candidate;
  }

  std::vector<Candidate> cppCandidates(
      const gsl::span<const FFICandidate> &c_candidates) {
    std::vector<Candidate> cpp_candidates;

    for (const auto &c_candidate : c_candidates) {
      cpp_candidates.push_back(cppCandidate(c_candidate));
    }

    return cpp_candidates;
  }

  RawSealPreCommitOutput cppRawSealPreCommitOutput(
      const FFISealPreCommitOutput &c_seal_pre_commit_output) {
    RawSealPreCommitOutput cpp_seal_pre_commit_output;

    std::copy(c_seal_pre_commit_output.comm_d,
              c_seal_pre_commit_output.comm_d + kCommitmentBytesLen,
              cpp_seal_pre_commit_output.comm_d.begin());
    std::copy(c_seal_pre_commit_output.comm_r,
              c_seal_pre_commit_output.comm_r + kCommitmentBytesLen,
              cpp_seal_pre_commit_output.comm_r.begin());

    return cpp_seal_pre_commit_output;
  }

  WriteWithoutAlignmentResult cppWriteWithoutAlignmentResult(
      const WriteWithoutAlignmentResponse &response) {
    WriteWithoutAlignmentResult result;

    result.total_write_unpadded = response.total_write_unpadded;
    std::copy(response.comm_p,
              response.comm_p + kCommitmentBytesLen,
              result.comm_p.begin());

    return result;
  }

  WriteWithAlignmentResult cppWriteWithAlignmentResult(
      const WriteWithAlignmentResponse &response) {
    WriteWithAlignmentResult result;

    result.left_alignment_unpadded = response.left_alignment_unpadded;
    result.total_write_unpadded = response.total_write_unpadded;
    std::copy(response.comm_p,
              response.comm_p + kCommitmentBytesLen,
              result.comm_p.begin());

    return result;
  }

  // ******************
  // TO С CASTED FUNCTIONS
  // ******************

  auto cPointerToArray(const Blob<32> &arr) {
    return reinterpret_cast<const uint8_t(*)[32]>(arr.data());
  }

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
    std::copy(cpp_candidate.partial_ticket.begin(),
              cpp_candidate.partial_ticket.end(),
              c_candidate.partial_ticket);
    std::copy(cpp_candidate.ticket.begin(),
              cpp_candidate.ticket.end(),
              c_candidate.ticket);
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

    std::copy(cpp_private_replica_info.comm_r.begin(),
              cpp_private_replica_info.comm_r.end(),
              c_private_replica_info.comm_r);

    return c_private_replica_info;
  }

  std::vector<FFIPrivateReplicaInfo> cPrivateReplicasInfo(
      gsl::span<const PrivateReplicaInfo> cpp_private_replicas_info) {
    std::vector<FFIPrivateReplicaInfo> c_private_replicas_info;
    for (const auto &cpp_private_replica_info : cpp_private_replicas_info) {
      c_private_replicas_info.push_back(
          cPrivateReplicaInfo(cpp_private_replica_info));
    }
    return c_private_replicas_info;
  }

  FFIPublicPieceInfo cPublicPieceInfo(
      const PublicPieceInfo &cpp_public_piece_info) {
    FFIPublicPieceInfo c_public_piece_info;

    c_public_piece_info.num_bytes = cpp_public_piece_info.size;
    std::copy(cpp_public_piece_info.comm_p.begin(),
              cpp_public_piece_info.comm_p.end(),
              c_public_piece_info.comm_p);

    return c_public_piece_info;
  }

  std::vector<FFIPublicPieceInfo> cPublicPiecesInfo(
      gsl::span<const PublicPieceInfo> cpp_public_pieces_info) {
    std::vector<FFIPublicPieceInfo> c_public_pieces_info;
    for (const auto &cpp_public_piece_info : cpp_public_pieces_info) {
      c_public_pieces_info.push_back(cPublicPieceInfo(cpp_public_piece_info));
    }
    return c_public_pieces_info;
  }

  // ******************
  // VERIFIED FUNCTIONS
  // ******************

  outcome::result<bool> Proofs::verifyPoSt(
      const uint64_t sector_size,
      const SortedPublicSectorInfo &sector_info,
      const Randomness &randomness,
      const uint64_t challenge_count,
      gsl::span<const uint8_t> proof,
      gsl::span<const Candidate> winners,
      const Blob<32> &prover_id) {
    std::vector<uint64_t> sorted_sector_ids;
    std::vector<uint8_t> flattening;
    for (size_t i = 0; i < sector_info.values.size(); i++) {
      sorted_sector_ids.push_back(sector_info.values[i].sector_id);
      std::copy(sector_info.values[i].comm_r.begin(),
                sector_info.values[i].comm_r.end(),
                back_inserter(flattening));
    }

    std::vector<FFICandidate> c_winners = cCandidates(winners);

    auto res_ptr = make_unique(verify_post(sector_size,
                                           cPointerToArray(randomness),
                                           challenge_count,
                                           sorted_sector_ids.data(),
                                           sorted_sector_ids.size(),
                                           flattening.data(),
                                           flattening.size(),
                                           proof.data(),
                                           proof.size(),
                                           c_winners.data(),
                                           c_winners.size(),
                                           cPointerToArray(prover_id)),
                               destroy_verify_post_response);

    if (res_ptr->status_code != 0) {
      logger->error("verifyPoSt: " + std::string(res_ptr->error_msg));
      return ProofsError::UNKNOWN;
    }

    return res_ptr->is_valid;
  }

  // ******************
  // GENERATED FUNCTIONS
  // ******************

  outcome::result<std::vector<Candidate>> Proofs::generateCandidates(
      const uint64_t sector_size,
      const Blob<32> &prover_id,
      const Randomness &randomness,
      const uint64_t challenge_count,
      const SortedPrivateReplicaInfo &sorted_private_replica_info) {
    std::vector<FFIPrivateReplicaInfo> c_sorted_private_sector_info =
        cPrivateReplicasInfo(sorted_private_replica_info.values);

    auto res_ptr =
        make_unique(generate_candidates(sector_size,
                                        cPointerToArray(randomness),
                                        challenge_count,
                                        c_sorted_private_sector_info.data(),
                                        c_sorted_private_sector_info.size(),
                                        cPointerToArray(prover_id)),
                    destroy_generate_candidates_response);
    if (res_ptr->status_code != 0) {
      logger->error("generateCandidates: " + std::string(res_ptr->error_msg));
      return ProofsError::UNKNOWN;
    }

    return cppCandidates(gsl::span<const FFICandidate>(
        res_ptr->candidates_ptr, res_ptr->candidates_len));
  }

  outcome::result<std::vector<uint8_t>> Proofs::generatePoSt(
      const uint64_t sectorSize,
      const Blob<32> &prover_id,
      const SortedPrivateReplicaInfo &private_replica_info,
      const Randomness &randomness,
      gsl::span<const Candidate> winners) {
    std::vector<FFICandidate> c_winners = cCandidates(winners);

    std::vector<FFIPrivateReplicaInfo> c_sorted_private_sector_info =
        cPrivateReplicasInfo(private_replica_info.values);

    auto res_ptr =
        make_unique(generate_post(sectorSize,
                                  cPointerToArray(randomness),
                                  c_sorted_private_sector_info.data(),
                                  c_sorted_private_sector_info.size(),
                                  c_winners.data(),
                                  c_winners.size(),
                                  cPointerToArray(prover_id)),
                    destroy_generate_post_response);

    if (res_ptr->status_code != 0) {
      logger->error("generatePoSt: " + std::string(res_ptr->error_msg));
      return ProofsError::UNKNOWN;
    }

    return std::vector<uint8_t>(
        res_ptr->flattened_proofs_ptr,
        res_ptr->flattened_proofs_ptr + res_ptr->flattened_proofs_len);
  }

  outcome::result<Comm> Proofs::generatePieceCommitmentFromFile(
      const std::string &piece_file_path, const uint64_t piece_size) {
    int fd;
    if ((fd = open(piece_file_path.c_str(), O_RDWR)) == -1) {
      return ProofsError::CANNOT_OPEN_FILE;
    }

    auto res_ptr = make_unique(generate_piece_commitment(fd, piece_size),
                               destroy_generate_piece_commitment_response);

    if (close(fd))
      logger->warn("generatePieceCommitmentFromFile: error in closing file "
                   + piece_file_path);

    if (res_ptr->status_code != 0) {
      logger->error("generatePieceCommitmentFromFile: "
                    + std::string(res_ptr->error_msg));
      return ProofsError::UNKNOWN;
    }

    return cppCommitment(gsl::span(res_ptr->comm_p, kCommitmentBytesLen));
  }

  outcome::result<Comm> Proofs::generateDataCommitment(
      const uint64_t sector_size, gsl::span<const PublicPieceInfo> pieces) {
    std::vector<FFIPublicPieceInfo> c_pieces = cPublicPiecesInfo(pieces);

    auto res_ptr = make_unique(
        generate_data_commitment(sector_size, c_pieces.data(), c_pieces.size()),
        destroy_generate_data_commitment_response);

    if (res_ptr->status_code != 0) {
      logger->error("generateDataCommitment: "
                    + std::string(res_ptr->error_msg));
      return ProofsError::UNKNOWN;
    }

    return cppCommitment(gsl::span(res_ptr->comm_d, kCommitmentBytesLen));
  }

  outcome::result<WriteWithoutAlignmentResult> Proofs::writeWithoutAlignment(
      const std::string &piece_file_path,
      const uint64_t piece_bytes,
      const std::string &staged_sector_file_path) {
    int piece_fd;
    if ((piece_fd = open(piece_file_path.c_str(), O_RDWR)) == -1) {
      return ProofsError::CANNOT_OPEN_FILE;
    }
    int staged_sector_fd;
    if ((staged_sector_fd = open(staged_sector_file_path.c_str(), O_RDWR))
        == -1) {
      if (close(piece_fd))
        logger->warn("writeWithoutAlignment: error in closing file "
                     + piece_file_path);
      return ProofsError::CANNOT_OPEN_FILE;
    }

    auto res_ptr = make_unique(
        write_without_alignment(piece_fd, piece_bytes, staged_sector_fd),
        destroy_write_without_alignment_response);

    if (close(piece_fd))
      logger->warn("writeWithoutAlignment: error in closing file "
                   + piece_file_path);
    if (close(staged_sector_fd))
      logger->warn("writeWithoutAlignment: error in closing file "
                   + staged_sector_file_path);
    if (res_ptr->status_code != 0) {
      logger->error("writeWithoutAlignment: "
                    + std::string(res_ptr->error_msg));

      return ProofsError::UNKNOWN;
    }

    return cppWriteWithoutAlignmentResult(*res_ptr);
  }

  outcome::result<WriteWithAlignmentResult> Proofs::writeWithAlignment(
      const std::string &piece_file_path,
      const uint64_t piece_bytes,
      const std::string &staged_sector_file_path,
      gsl::span<const uint64_t> existing_piece_sizes) {
    int piece_fd;
    if ((piece_fd = open(piece_file_path.c_str(), O_RDWR)) == -1) {
      return ProofsError::CANNOT_OPEN_FILE;
    }
    int staged_sector_fd;
    if ((staged_sector_fd = open(staged_sector_file_path.c_str(), O_RDWR))
        == -1) {
      if (close(piece_fd))
        logger->warn("writeWithAlignment: error in closing file "
                     + piece_file_path);
      return ProofsError::CANNOT_OPEN_FILE;
    }
    auto res_ptr =
        make_unique(write_with_alignment(piece_fd,
                                         piece_bytes,
                                         staged_sector_fd,
                                         existing_piece_sizes.data(),
                                         existing_piece_sizes.size()),
                    destroy_write_with_alignment_response);

    if (res_ptr->status_code != 0) {
      logger->error("writeWithAlignment: " + std::string(res_ptr->error_msg));
      return ProofsError::UNKNOWN;
    }
    if (close(piece_fd))
      logger->warn("writeWithAlignment: error in closing file "
                   + piece_file_path);
    if (close(staged_sector_fd))
      logger->warn("writeWithAlignment: error in closing file "
                   + staged_sector_file_path);
    return cppWriteWithAlignmentResult(*res_ptr);
  }

  outcome::result<RawSealPreCommitOutput> Proofs::sealPreCommit(
      const uint64_t sector_size,
      const uint8_t porep_proof_partitions,
      const std::string &cache_dir_path,
      const std::string &staged_sector_path,
      const std::string &sealed_sector_path,
      const uint64_t sector_id,
      const Blob<32> &prover_id,
      const Blob<32> &ticket,
      gsl::span<const PublicPieceInfo> pieces) {
    std::vector<FFIPublicPieceInfo> c_pieces = cPublicPiecesInfo(pieces);

    auto res_ptr = make_unique(
        seal_pre_commit(cSectorClass(sector_size, porep_proof_partitions),
                        cache_dir_path.c_str(),
                        staged_sector_path.c_str(),
                        sealed_sector_path.c_str(),
                        sector_id,
                        cPointerToArray(prover_id),
                        cPointerToArray(ticket),
                        c_pieces.data(),
                        c_pieces.size()),
        destroy_seal_pre_commit_response);
    if (res_ptr->status_code != 0) {
      logger->error("sealPreCommit: " + std::string(res_ptr->error_msg));
      return ProofsError::UNKNOWN;
    }

    return cppRawSealPreCommitOutput(res_ptr->seal_pre_commit_output);
  }

  SortedPrivateReplicaInfo Proofs::newSortedPrivateReplicaInfo(
      gsl::span<const PrivateReplicaInfo> replica_info) {
    SortedPrivateReplicaInfo sorted_replica_info;
    sorted_replica_info.values.assign(replica_info.cbegin(),
                                      replica_info.cend());
    std::sort(sorted_replica_info.values.begin(),
              sorted_replica_info.values.end(),
              [](const PrivateReplicaInfo &lhs, const PrivateReplicaInfo &rhs) {
                return std::memcmp(lhs.comm_r.data(),
                                   rhs.comm_r.data(),
                                   lhs.comm_r.size())
                       < 0;
              });

    return sorted_replica_info;
  }

  SortedPublicSectorInfo Proofs::newSortedPublicSectorInfo(
      gsl::span<const PublicSectorInfo> sector_info) {
    SortedPublicSectorInfo sorted_sector_info;
    sorted_sector_info.values.assign(sector_info.cbegin(), sector_info.cend());
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
