/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proofs/proofs.hpp"

#include <fcntl.h>
#include <filecoin-ffi/filecoin.h>
#include "boost/filesystem/fstream.hpp"
#include "primitives/cid/comm_cid.hpp"
#include "proofs/proofs_error.hpp"

namespace fc::proofs {

  common::Logger Proofs::logger_ = common::createLogger("proofs");

  using common::Blob;
  using crypto::randomness::Randomness;
  using namespace fc::common;
  using primitives::sector::SectorId;

  template <typename T, typename D>
  auto make_unique(T *ptr, D deleter) {
    return std::unique_ptr<T, D>(ptr, deleter);
  }

  // ******************
  // TO CPP CASTED FUNCTIONS
  // ******************

  PoStCandidateWithTicket cppCandidateWithTicket(
      const FFICandidate &c_candidate) {
    return PoStCandidateWithTicket{
        .candidate =
            PoStCandidate{
                .sector =
                    SectorId{
                        .miner = 0,
                        .sector = c_candidate.sector_id,
                    },
                .partial_ticket = cppCommitment(
                    gsl::make_span(c_candidate.partial_ticket, 32)),
                .challenge_index = int64_t(c_candidate.sector_challenge_index),
            },
        .ticket = cppCommitment(gsl::make_span(c_candidate.ticket, 32)),
    };
  }

  std::vector<PoStCandidateWithTicket> cppCandidatesWithTickets(
      gsl::span<const FFICandidate> c_candidates) {
    std::vector<PoStCandidateWithTicket> cpp_candidates;
    for (const auto &c_candidate : c_candidates) {
      cpp_candidates.push_back(cppCandidateWithTicket(c_candidate));
    }
    return cpp_candidates;
  }

  WriteWithoutAlignmentResult cppWriteWithoutAlignmentResult(
      const WriteWithoutAlignmentResponse &response) {
    WriteWithoutAlignmentResult result;

    result.total_write_unpadded = response.total_write_unpadded;
    result.piece_cid =
        pieceCommitmentV1ToCID(gsl::span(response.comm_p, kCommitmentBytesLen));

    return result;
  }

  WriteWithAlignmentResult cppWriteWithAlignmentResult(
      const WriteWithAlignmentResponse &response) {
    WriteWithAlignmentResult result;

    result.left_alignment_unpadded = response.left_alignment_unpadded;
    result.total_write_unpadded = response.total_write_unpadded;
    result.piece_cid =
        pieceCommitmentV1ToCID(gsl::span(response.comm_p, kCommitmentBytesLen));

    return result;
  }

  // ******************
  // TO С CASTED FUNCTIONS
  // ******************

  outcome::result<FFIRegisteredPoStProof> cRegisteredPoStProof(
      RegisteredProof proof_type) {
    switch (proof_type) {
      case RegisteredProof::StackedDRG1KiBPoSt:
        return FFIRegisteredPoStProof_StackedDrg1KiBV1;
      case RegisteredProof::StackedDRG16MiBPoSt:
        return FFIRegisteredPoStProof_StackedDrg16MiBV1;
      case RegisteredProof::StackedDRG256MiBPoSt:
        return FFIRegisteredPoStProof_StackedDrg256MiBV1;
      case RegisteredProof::StackedDRG1GiBPoSt:
        return FFIRegisteredPoStProof_StackedDrg1GiBV1;
      case RegisteredProof::StackedDRG32GiBPoSt:
        return FFIRegisteredPoStProof_StackedDrg32GiBV1;
      default:
        return ProofsError::NO_SUCH_POST_PROOF;
    }
  }

  outcome::result<FFIRegisteredSealProof> cRegisteredSealProof(
      RegisteredProof proof_type) {
    switch (proof_type) {
      case RegisteredProof::StackedDRG1KiBSeal:
        return FFIRegisteredSealProof_StackedDrg1KiBV1;
      case RegisteredProof::StackedDRG16MiBSeal:
        return FFIRegisteredSealProof_StackedDrg16MiBV1;
      case RegisteredProof::StackedDRG256MiBSeal:
        return FFIRegisteredSealProof_StackedDrg256MiBV1;
      case RegisteredProof::StackedDRG1GiBSeal:
        return FFIRegisteredSealProof_StackedDrg1GiBV1;
      case RegisteredProof::StackedDRG32GiBSeal:
        return FFIRegisteredSealProof_StackedDrg32GiBV1;
      default:
        return ProofsError::NO_SUCH_SEAL_PROOF;
    }
  }

  auto cPointerToArray(const Blob<32> &arr) {
    // NOLINTNEXTLINE
    return reinterpret_cast<const uint8_t(*)[32]>(arr.data());
  }

  FFICandidate cCandidate(const PoStCandidate &cpp_candidate) {
    FFICandidate c_candidate;
    c_candidate.sector_id = cpp_candidate.sector.sector;
    c_candidate.sector_challenge_index =
        uint64_t(cpp_candidate.challenge_index);
    std::copy(cpp_candidate.partial_ticket.begin(),
              cpp_candidate.partial_ticket.end(),
              c_candidate.partial_ticket);
    std::copy(cpp_candidate.partial_ticket.begin(),
              cpp_candidate.partial_ticket.end(),
              c_candidate.ticket);
    return c_candidate;
  }

  std::vector<FFICandidate> cCandidates(
      gsl::span<const PoStCandidate> cpp_candidates) {
    std::vector<FFICandidate> c_candidates;
    for (const auto &cpp_candidate : cpp_candidates) {
      c_candidates.push_back(cCandidate(cpp_candidate));
    }
    return c_candidates;
  }

  outcome::result<FFIPrivateReplicaInfo> cPrivateReplicaInfo(
      const PrivateSectorInfo &cpp_private_replica_info) {
    FFIPrivateReplicaInfo c_private_replica_info;

    c_private_replica_info.sector_id = cpp_private_replica_info.sector;

    c_private_replica_info.cache_dir_path =
        cpp_private_replica_info.cache_dir_path.data();
    c_private_replica_info.replica_path =
        cpp_private_replica_info.sealed_sector_path.data();
    OUTCOME_TRY(c_proof_type,
                cRegisteredPoStProof(cpp_private_replica_info.post_proof_type));

    c_private_replica_info.registered_proof = c_proof_type;

    OUTCOME_TRY(comm_r,
                CIDToReplicaCommitmentV1(cpp_private_replica_info.sealed_cid));
    std::copy(comm_r.begin(), comm_r.end(), c_private_replica_info.comm_r);

    return c_private_replica_info;
  }

  outcome::result<std::vector<FFIPrivateReplicaInfo>> cPrivateReplicasInfo(
      gsl::span<const PrivateSectorInfo> cpp_private_replicas_info) {
    std::vector<FFIPrivateReplicaInfo> c_private_replicas_info;
    for (const auto &cpp_private_replica_info : cpp_private_replicas_info) {
      OUTCOME_TRY(c_private_replica_info,
                  cPrivateReplicaInfo(cpp_private_replica_info));
      c_private_replicas_info.push_back(c_private_replica_info);
    }
    return c_private_replicas_info;
  }

  outcome::result<FFIPublicReplicaInfo> cPublicReplicaInfo(
      const PublicSectorInfo &cpp_public_replica_info) {
    FFIPublicReplicaInfo c_public_replica_info{
        .sector_id = cpp_public_replica_info.sector_num};

    OUTCOME_TRY(c_proof_type,
                cRegisteredPoStProof(cpp_public_replica_info.post_proof_type));

    c_public_replica_info.registered_proof = c_proof_type;

    OUTCOME_TRY(comm_r,
                CIDToReplicaCommitmentV1(cpp_public_replica_info.sealed_cid));
    std::copy(comm_r.begin(), comm_r.end(), c_public_replica_info.comm_r);

    return c_public_replica_info;
  }

  outcome::result<std::vector<FFIPublicReplicaInfo>> cPublicReplicasInfo(
      gsl::span<const PublicSectorInfo> cpp_public_replicas_info) {
    std::vector<FFIPublicReplicaInfo> c_public_replicas_info;
    for (const auto &cpp_public_replica_info : cpp_public_replicas_info) {
      OUTCOME_TRY(c_public_replica_info,
                  cPublicReplicaInfo(cpp_public_replica_info));
      c_public_replicas_info.push_back(c_public_replica_info);
    }
    return c_public_replicas_info;
  }

  outcome::result<FFIPublicPieceInfo> cPublicPieceInfo(
      const PieceInfo &cpp_public_piece_info) {
    FFIPublicPieceInfo c_public_piece_info;

    c_public_piece_info.num_bytes = cpp_public_piece_info.size.unpadded();
    OUTCOME_TRY(comm_p, CIDToPieceCommitmentV1(cpp_public_piece_info.cid));
    std::copy(comm_p.begin(), comm_p.end(), c_public_piece_info.comm_p);

    return c_public_piece_info;
  }

  outcome::result<std::vector<FFIPublicPieceInfo>> cPublicPiecesInfo(
      gsl::span<const PieceInfo> cpp_public_pieces_info) {
    std::vector<FFIPublicPieceInfo> c_public_pieces_info;
    for (const auto &cpp_public_piece_info : cpp_public_pieces_info) {
      OUTCOME_TRY(c_piblic_piece_info, cPublicPieceInfo(cpp_public_piece_info));
      c_public_pieces_info.push_back(c_piblic_piece_info);
    }
    return c_public_pieces_info;
  }

  // ******************
  // VERIFIED FUNCTIONS
  // ******************

  outcome::result<bool> Proofs::verifyPoSt(
      const SortedPublicSectorInfo &public_sector_info,
      const PoStRandomness &randomness,
      uint64_t challenge_count,
      gsl::span<const uint8_t> proof,
      gsl::span<const PoStCandidate> winners,
      const Prover &prover_id) {
    OUTCOME_TRY(c_public_sector_info,
                cPublicReplicasInfo(public_sector_info.values));

    std::vector<FFICandidate> c_winners = cCandidates(winners);

    auto res_ptr = make_unique(verify_post(cPointerToArray(randomness),
                                           challenge_count,
                                           c_public_sector_info.data(),
                                           c_public_sector_info.size(),
                                           proof.data(),
                                           proof.size(),
                                           c_winners.data(),
                                           c_winners.size(),
                                           cPointerToArray(prover_id)),
                               destroy_verify_post_response);

    if (res_ptr->status_code != 0) {
      logger_->error("verifyPoSt: " + std::string(res_ptr->error_msg));
      return ProofsError::UNKNOWN;
    }

    return res_ptr->is_valid;
  }

  // ******************
  // GENERATED FUNCTIONS
  // ******************

  outcome::result<std::vector<PoStCandidateWithTicket>>
  Proofs::generateCandidates(
      const Prover &prover_id,
      const PoStRandomness &randomness,
      uint64_t challenge_count,
      const SortedPrivateSectorInfo &sorted_private_replica_info) {
    OUTCOME_TRY(c_sorted_private_sector_info,
                cPrivateReplicasInfo(sorted_private_replica_info.values));

    auto res_ptr =
        make_unique(generate_candidates(cPointerToArray(randomness),
                                        challenge_count,
                                        c_sorted_private_sector_info.data(),
                                        c_sorted_private_sector_info.size(),
                                        cPointerToArray(prover_id)),
                    destroy_generate_candidates_response);

    if (res_ptr->status_code != 0) {
      logger_->error("generateCandidates: " + std::string(res_ptr->error_msg));
      return ProofsError::UNKNOWN;
    }

    return cppCandidatesWithTickets(gsl::span<const FFICandidate>(
        res_ptr->candidates_ptr, res_ptr->candidates_len));
  }

  outcome::result<Proof> Proofs::generatePoSt(
      const Prover &prover_id,
      const SortedPrivateSectorInfo &private_replica_info,
      const PoStRandomness &randomness,
      gsl::span<const PoStCandidate> winners) {
    std::vector<FFICandidate> c_winners = cCandidates(winners);

    OUTCOME_TRY(c_sorted_private_sector_info,
                cPrivateReplicasInfo(private_replica_info.values));

    auto res_ptr =
        make_unique(generate_post(cPointerToArray(randomness),
                                  c_sorted_private_sector_info.data(),
                                  c_sorted_private_sector_info.size(),
                                  c_winners.data(),
                                  c_winners.size(),
                                  cPointerToArray(prover_id)),
                    destroy_generate_post_response);

    if (res_ptr->status_code != 0) {
      logger_->error("generatePoSt: " + std::string(res_ptr->error_msg));
      return ProofsError::UNKNOWN;
    }

    return Proof(res_ptr->flattened_proofs_ptr,
                 res_ptr->flattened_proofs_ptr
                     + res_ptr->flattened_proofs_len);  // NOLINT
  }

  outcome::result<WriteWithoutAlignmentResult> Proofs::writeWithoutAlignment(
      RegisteredProof proof_type,
      const std::string &piece_file_path,
      const UnpaddedPieceSize &piece_bytes,
      const std::string &staged_sector_file_path) {
    OUTCOME_TRY(c_proof_type, cRegisteredSealProof(proof_type));

    int piece_fd;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
    if ((piece_fd = open(piece_file_path.c_str(), O_RDWR)) == -1) {
      return ProofsError::CANNOT_OPEN_FILE;
    }
    int staged_sector_fd;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
    if ((staged_sector_fd = open(staged_sector_file_path.c_str(), O_RDWR))
        == -1) {
      // NOLINTNEXTLINE(readability-implicit-bool-conversion)
      if (close(piece_fd))
        logger_->warn("writeWithoutAlignment: error in closing file "
                      + piece_file_path);
      return ProofsError::CANNOT_OPEN_FILE;
    }

    auto res_ptr =
        make_unique(write_without_alignment(
                        c_proof_type, piece_fd, piece_bytes, staged_sector_fd),
                    destroy_write_without_alignment_response);

    // NOLINTNEXTLINE(readability-implicit-bool-conversion)
    if (close(piece_fd))
      logger_->warn("writeWithoutAlignment: error in closing file "
                    + piece_file_path);
    // NOLINTNEXTLINE(readability-implicit-bool-conversion)
    if (close(staged_sector_fd))
      logger_->warn("writeWithoutAlignment: error in closing file "
                    + staged_sector_file_path);
    if (res_ptr->status_code != 0) {
      logger_->error("writeWithoutAlignment: "
                     + std::string(res_ptr->error_msg));

      return ProofsError::UNKNOWN;
    }

    return cppWriteWithoutAlignmentResult(*res_ptr);
  }

  outcome::result<WriteWithAlignmentResult> Proofs::writeWithAlignment(
      RegisteredProof proof_type,
      const std::string &piece_file_path,
      const UnpaddedPieceSize &piece_bytes,
      const std::string &staged_sector_file_path,
      gsl::span<const uint64_t> existing_piece_sizes) {
    OUTCOME_TRY(c_proof_type, cRegisteredSealProof(proof_type));
    int piece_fd;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
    if ((piece_fd = open(piece_file_path.c_str(), O_RDWR)) == -1) {
      return ProofsError::CANNOT_OPEN_FILE;
    }
    int staged_sector_fd;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
    if ((staged_sector_fd = open(staged_sector_file_path.c_str(), O_RDWR))
        == -1) {
      // NOLINTNEXTLINE(readability-implicit-bool-conversion)
      if (close(piece_fd))
        logger_->warn("writeWithAlignment: error in closing file "
                      + piece_file_path);
      return ProofsError::CANNOT_OPEN_FILE;
    }

    auto res_ptr =
        make_unique(write_with_alignment(c_proof_type,
                                         piece_fd,
                                         uint64_t(piece_bytes),
                                         staged_sector_fd,
                                         existing_piece_sizes.data(),
                                         existing_piece_sizes.size()),
                    destroy_write_with_alignment_response);

    if (res_ptr->status_code != 0) {
      logger_->error("writeWithAlignment: " + std::string(res_ptr->error_msg));
      return ProofsError::UNKNOWN;
    }
    // NOLINTNEXTLINE(readability-implicit-bool-conversion)
    if (close(piece_fd))
      logger_->warn("writeWithAlignment: error in closing file "
                    + piece_file_path);
    // NOLINTNEXTLINE(readability-implicit-bool-conversion)
    if (close(staged_sector_fd))
      logger_->warn("writeWithAlignment: error in closing file "
                    + staged_sector_file_path);
    return cppWriteWithAlignmentResult(*res_ptr);
  }

  outcome::result<Phase1Output> Proofs::sealPreCommitPhase1(
      RegisteredProof proof_type,
      const std::string &cache_dir_path,
      const std::string &staged_sector_path,
      const std::string &sealed_sector_path,
      SectorNumber sector_num,
      const Prover &prover_id,
      const SealRandomness &ticket,
      gsl::span<const PieceInfo> pieces) {
    OUTCOME_TRY(c_proof_type, cRegisteredSealProof(proof_type));

    OUTCOME_TRY(c_pieces, cPublicPiecesInfo(pieces));

    auto res_ptr =
        make_unique(seal_pre_commit_phase1(c_proof_type,
                                           cache_dir_path.c_str(),
                                           staged_sector_path.c_str(),
                                           sealed_sector_path.c_str(),
                                           sector_num,
                                           cPointerToArray(prover_id),
                                           cPointerToArray(ticket),
                                           c_pieces.data(),
                                           c_pieces.size()),

                    destroy_seal_pre_commit_phase1_response);

    if (res_ptr->status_code != 0) {
      logger_->error("Seal precommit phase 1: "
                     + std::string(res_ptr->error_msg));
      return ProofsError::UNKNOWN;
    }

    return Phase1Output(
        res_ptr->seal_pre_commit_phase1_output_ptr,
        res_ptr->seal_pre_commit_phase1_output_ptr
            + res_ptr->seal_pre_commit_phase1_output_len);  // NOLINT
  }

  outcome::result<std::pair<CID, CID>> Proofs::sealPreCommitPhase2(
      gsl::span<const uint8_t> phase1_output,
      const std::string &cache_dir_path,
      const std::string &sealed_sector_path) {
    auto res_ptr =
        make_unique(seal_pre_commit_phase2(phase1_output.data(),
                                           phase1_output.size(),
                                           cache_dir_path.c_str(),
                                           sealed_sector_path.c_str()),
                    destroy_seal_pre_commit_phase2_response);

    if (res_ptr->status_code != 0) {
      logger_->error("Seal precommit phase 2: "
                     + std::string(res_ptr->error_msg));
      return ProofsError::UNKNOWN;
    }

    return std::make_pair(replicaCommitmentV1ToCID(gsl::make_span(
                              res_ptr->comm_r, kCommitmentBytesLen)),
                          dataCommitmentV1ToCID(gsl::make_span(
                              res_ptr->comm_d, kCommitmentBytesLen)));
  }

  SortedPrivateSectorInfo Proofs::newSortedPrivateSectorInfo(
      gsl::span<const PrivateSectorInfo> replica_info) {
    SortedPrivateSectorInfo sorted_replica_info;
    sorted_replica_info.values.assign(replica_info.cbegin(),
                                      replica_info.cend());
    std::sort(sorted_replica_info.values.begin(),
              sorted_replica_info.values.end(),
              [](const PrivateSectorInfo &lhs, const PrivateSectorInfo &rhs) {
                return lhs.sealed_cid < rhs.sealed_cid;
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
                return lhs.sealed_cid < rhs.sealed_cid;
              });

    return sorted_sector_info;
  }

  outcome::result<CID> Proofs::generatePieceCIDFromFile(
      RegisteredProof proof_type,
      const std::string &piece_file_path,
      UnpaddedPieceSize piece_size) {
    OUTCOME_TRY(c_proof_type, cRegisteredSealProof(proof_type));

    int piece_fd;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
    if ((piece_fd = open(piece_file_path.c_str(), O_RDWR)) == -1) {
      return ProofsError::CANNOT_OPEN_FILE;
    }

    auto res_ptr = make_unique(
        generate_piece_commitment(c_proof_type, piece_fd, piece_size),
        destroy_generate_piece_commitment_response);

    // NOLINTNEXTLINE(readability-implicit-bool-conversion)
    if (close(piece_fd))
      logger_->warn("generatePieceCIDFromFile: error in closing file "
                    + piece_file_path);

    if (res_ptr->status_code != 0) {
      logger_->error("GeneratePieceCIDFromFile: "
                     + std::string(res_ptr->error_msg));
      return ProofsError::UNKNOWN;
    }

    return dataCommitmentV1ToCID(
        gsl::make_span(res_ptr->comm_p, kCommitmentBytesLen));
  }

  outcome::result<CID> Proofs::generateUnsealedCID(
      RegisteredProof proof_type, gsl::span<PieceInfo> pieces) {
    OUTCOME_TRY(c_proof_type, cRegisteredSealProof(proof_type));

    OUTCOME_TRY(c_pieces, cPublicPiecesInfo(pieces));

    auto res_ptr =
        make_unique(generate_data_commitment(
                        c_proof_type, c_pieces.data(), c_pieces.size()),
                    destroy_generate_data_commitment_response);

    if (res_ptr->status_code != 0) {
      logger_->error("generateUnsealedCID: " + std::string(res_ptr->error_msg));
      return ProofsError::UNKNOWN;
    }

    return dataCommitmentV1ToCID(
        gsl::make_span(res_ptr->comm_d, kCommitmentBytesLen));
  }

}  // namespace fc::proofs
