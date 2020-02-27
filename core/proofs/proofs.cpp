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

  template <typename T, typename D>
  auto make_unique(T *ptr, D deleter) {
    return std::unique_ptr<T, D>(ptr, deleter);
  }

  // ******************
  // TO CPP CASTED FUNCTIONS
  // ******************

  /*Candidate cppCandidate(const FFICandidate &c_candidate) {
    Candidate candidate;
    candidate.sector_id = c_candidate.sector_id;
    candidate.sector_challenge_index = c_candidate.sector_challenge_index;
    std::copy(c_candidate.ticket,
              // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
              c_candidate.ticket + Ticket::size(),
              candidate.ticket.begin());
    std::copy(c_candidate.partial_ticket,
              // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
              c_candidate.partial_ticket + Ticket::size(),
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
              // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
              c_seal_pre_commit_output.comm_d + kCommitmentBytesLen,
              cpp_seal_pre_commit_output.comm_d.begin());
    std::copy(c_seal_pre_commit_output.comm_r,
              // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
              c_seal_pre_commit_output.comm_r + kCommitmentBytesLen,
              cpp_seal_pre_commit_output.comm_r.begin());

    return cpp_seal_pre_commit_output;
  }*/

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

  /*FFISealPreCommitOutput cRawSealPreCommitOutput(
      const RawSealPreCommitOutput &cpp_seal_pre_commit_output) {
    FFISealPreCommitOutput c_seal_pre_commit_output{};

    std::copy(cpp_seal_pre_commit_output.comm_d.begin(),
              cpp_seal_pre_commit_output.comm_d.end(),
              c_seal_pre_commit_output.comm_d);
    std::copy(cpp_seal_pre_commit_output.comm_r.begin(),
              cpp_seal_pre_commit_output.comm_r.end(),
              c_seal_pre_commit_output.comm_r);

    return c_seal_pre_commit_output;
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
  }*/

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

  /*outcome::result<bool> Proofs::verifyPoSt(
      uint64_t sector_size,
      const SortedPublicSectorInfo &sector_info,
      const Randomness &randomness,
      uint64_t challenge_count,
      gsl::span<const uint8_t> proof,
      gsl::span<const Candidate> winners,
      const Prover &prover_id) {
    std::vector<uint64_t> sorted_sector_ids;
    std::vector<uint8_t> flattening;
    for (const auto &value : sector_info.values) {
      sorted_sector_ids.push_back(value.sector_id);
      std::copy(
          value.comm_r.begin(), value.comm_r.end(), back_inserter(flattening));
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
      logger_->error("verifyPoSt: " + std::string(res_ptr->error_msg));
      return ProofsError::UNKNOWN;
    }

    return res_ptr->is_valid;
  }

  outcome::result<bool> Proofs::verifySeal(uint64_t sector_size,
                                           const Comm &comm_r,
                                           const Comm &comm_d,
                                           const Prover &prover_id,
                                           const Ticket &ticket,
                                           const Seed &seed,
                                           uint64_t sector_id,
                                           gsl::span<const uint8_t> proof) {
    auto res_ptr = make_unique(verify_seal(sector_size,
                                           cPointerToArray(comm_r),
                                           cPointerToArray(comm_d),
                                           cPointerToArray(prover_id),
                                           cPointerToArray(ticket),
                                           cPointerToArray(seed),
                                           sector_id,
                                           proof.data(),
                                           proof.size()),
                               destroy_verify_seal_response);

    if (res_ptr->status_code != 0) {
      logger_->error("verifySeal: " + std::string(res_ptr->error_msg));

      return ProofsError::UNKNOWN;
    }

    return res_ptr->is_valid;
  }*/

  // ******************
  // GENERATED FUNCTIONS
  // ******************

  /*outcome::result<std::vector<Candidate>> Proofs::generateCandidates(
      uint64_t sector_size,
      const Prover &prover_id,
      const Randomness &randomness,
      uint64_t challenge_count,
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
      logger_->error("generateCandidates: " + std::string(res_ptr->error_msg));
      return ProofsError::UNKNOWN;
    }

    return cppCandidates(gsl::span<const FFICandidate>(
        res_ptr->candidates_ptr, res_ptr->candidates_len));
  }

  outcome::result<Proof> Proofs::generatePoSt(
      uint64_t sectorSize,
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
      logger_->error("generatePoSt: " + std::string(res_ptr->error_msg));
      return ProofsError::UNKNOWN;
    }

    return Proof(res_ptr->flattened_proofs_ptr,
                 res_ptr->flattened_proofs_ptr
                     + res_ptr->flattened_proofs_len);  // NOLINT
  }

  outcome::result<Comm> Proofs::generatePieceCommitmentFromFile(
      const std::string &piece_file_path, uint64_t piece_size) {
    int fd;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
    if ((fd = open(piece_file_path.c_str(), O_RDWR)) == -1) {
      return ProofsError::CANNOT_OPEN_FILE;
    }

    auto res_ptr = make_unique(generate_piece_commitment(fd, piece_size),
                               destroy_generate_piece_commitment_response);

    // NOLINTNEXTLINE(readability-implicit-bool-conversion)
    if (close(fd))
      logger_->warn("generatePieceCommitmentFromFile: error in closing file "
                    + piece_file_path);

    if (res_ptr->status_code != 0) {
      logger_->error("generatePieceCommitmentFromFile: "
                     + std::string(res_ptr->error_msg));
      return ProofsError::UNKNOWN;
    }

    return cppCommitment(gsl::span(res_ptr->comm_p, kCommitmentBytesLen));
  }

  outcome::result<Comm> Proofs::generateDataCommitment(
      uint64_t sector_size, gsl::span<const PublicPieceInfo> pieces) {
    std::vector<FFIPublicPieceInfo> c_pieces = cPublicPiecesInfo(pieces);

    auto res_ptr = make_unique(
        generate_data_commitment(sector_size, c_pieces.data(), c_pieces.size()),
        destroy_generate_data_commitment_response);

    if (res_ptr->status_code != 0) {
      logger_->error("generateDataCommitment: "
                     + std::string(res_ptr->error_msg));
      return ProofsError::UNKNOWN;
    }

    return cppCommitment(gsl::span(res_ptr->comm_d, kCommitmentBytesLen));
  }*/

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

  /*outcome::result<Proof> Proofs::sealCommit(
      uint64_t sector_size,
      uint8_t porep_proof_partitions,
      const std::string &cache_dir_path,
      uint64_t sector_id,
      const Prover &prover_id,
      const Ticket &ticket,
      const Seed &seed,
      gsl::span<const PublicPieceInfo> pieces,
      const RawSealPreCommitOutput &rspco) {
    std::vector<FFIPublicPieceInfo> c_pieces = cPublicPiecesInfo(pieces);

    auto res_ptr = make_unique(
        seal_commit(cSectorClass(sector_size, porep_proof_partitions),
                    cache_dir_path.c_str(),
                    sector_id,
                    cPointerToArray(prover_id),
                    cPointerToArray(ticket),
                    cPointerToArray(seed),
                    c_pieces.data(),
                    c_pieces.size(),
                    cRawSealPreCommitOutput(rspco)),
        destroy_seal_commit_response);

    if (res_ptr->status_code != 0) {
      logger_->error("sealCommit: " + std::string(res_ptr->error_msg));
      return ProofsError::UNKNOWN;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return Proof(res_ptr->proof_ptr, res_ptr->proof_ptr + res_ptr->proof_len);
  }

  outcome::result<void> Proofs::unseal(uint64_t sector_size,
                                       uint8_t porep_proof_partitions,
                                       const std::string &cache_dir_path,
                                       const std::string &sealed_sector_path,
                                       const std::string &unseal_output_path,
                                       uint64_t sector_id,
                                       const Prover &prover_id,
                                       const Ticket &ticket,
                                       const Comm &comm_d) {
    auto res_ptr =
        make_unique(::unseal(cSectorClass(sector_size, porep_proof_partitions),
                             cache_dir_path.c_str(),
                             sealed_sector_path.c_str(),
                             unseal_output_path.c_str(),
                             sector_id,
                             cPointerToArray(prover_id),
                             cPointerToArray(ticket),
                             cPointerToArray(comm_d)),
                    destroy_unseal_response);

    if (res_ptr->status_code != 0) {
      logger_->error("unseal: " + std::string(res_ptr->error_msg));

      return ProofsError::UNKNOWN;
    }

    return outcome::success();
  }

  outcome::result<void> Proofs::unsealRange(
      uint64_t sector_size,
      uint8_t porep_proof_partitions,
      const std::string &cache_dir_path,
      const std::string &sealed_sector_path,
      const std::string &unseal_output_path,
      uint64_t sector_id,
      const Prover &prover_id,
      const Ticket &ticket,
      const Comm &comm_d,
      uint64_t offset,
      uint64_t length) {
    auto res_ptr = make_unique(
        unseal_range(cSectorClass(sector_size, porep_proof_partitions),
                     cache_dir_path.c_str(),
                     sealed_sector_path.c_str(),
                     unseal_output_path.c_str(),
                     sector_id,
                     cPointerToArray(prover_id),
                     cPointerToArray(ticket),
                     cPointerToArray(comm_d),
                     offset,
                     length),
        destroy_unseal_range_response);

    if (res_ptr->status_code != 0) {
      logger_->error("unsealRange: " + std::string(res_ptr->error_msg));

      return ProofsError::UNKNOWN;
    }

    return outcome::success();
  }

  SortedPrivateReplicaInfo Proofs::newSortedPrivateReplicaInfo(
      gsl::span<const PrivateReplicaInfo> replica_info) {
    SortedPrivateReplicaInfo sorted_replica_info;
    sorted_replica_info.values.assign(replica_info.cbegin(),
                                      replica_info.cend());
    std::sort(sorted_replica_info.values.begin(),
              sorted_replica_info.values.end(),
              [](const PrivateReplicaInfo &lhs, const PrivateReplicaInfo &rhs) {
                return std::memcmp(
                           lhs.comm_r.data(), rhs.comm_r.data(), Comm::size())
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
                return std::memcmp(
                           lhs.comm_r.data(), rhs.comm_r.data(), Comm::size())
                       < 0;
              });

    return sorted_sector_info;
  }

  outcome::result<Ticket> Proofs::finalizeTicket(const Ticket &partial_ticket) {
    auto res_ptr = make_unique(finalize_ticket(cPointerToArray(partial_ticket)),
                               destroy_finalize_ticket_response);

    if (res_ptr->status_code != 0) {
      logger_->error("finalizeTicket: " + std::string(res_ptr->error_msg));
      return ProofsError::UNKNOWN;
    }

    static const int kTicketSize = 32;

    return cppCommitment(gsl::make_span(res_ptr->ticket, kTicketSize));
  }

  uint64_t Proofs::getMaxUserBytesPerStagedSector(uint64_t sector_size) {
    return get_max_user_bytes_per_staged_sector(sector_size);
  }

  outcome::result<Devices> Proofs::getGPUDevices() {
    auto res_ptr = make_unique(get_gpu_devices(), destroy_gpu_device_response);

    if (res_ptr->status_code != 0) {
      logger_->error("getGPUDevices: " + std::string(res_ptr->error_msg));
      return ProofsError::UNKNOWN;
    }

    if (res_ptr->devices_ptr == nullptr || res_ptr->devices_len == 0) {
      return Devices();
    }

    return Devices(res_ptr->devices_ptr,
                   res_ptr->devices_ptr + res_ptr->devices_len);  // NOLINT
  }*/

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
