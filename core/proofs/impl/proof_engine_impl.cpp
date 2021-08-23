/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proofs/impl/proof_engine_impl.hpp"

#include <filecoin-ffi/filcrypto.h>
#include <thread>

#include "codec/uvarint.hpp"
#include "common/ffi.hpp"
#include "primitives/cid/comm_cid.hpp"
#include "proofs/proofs_error.hpp"
#include "sector_storage/zerocomm/zerocomm.hpp"

#define PROOFS_TRY(label)                                     \
  do {                                                        \
    if (res_ptr->status_code                                  \
        != FCPResponseStatus::FCPResponseStatus_FCPNoError) { \
      logger_->error("{}: {}", label, res_ptr->error_msg);    \
      return cppResponseStatus(res_ptr->status_code);         \
    }                                                         \
  } while (0)

namespace fc::proofs {
  namespace ffi = common::ffi;

  using primitives::cid::CIDToDataCommitmentV1;
  using primitives::cid::CIDToPieceCommitmentV1;
  using primitives::cid::CIDToReplicaCommitmentV1;
  using primitives::cid::dataCommitmentV1ToCID;
  using primitives::cid::kCommitmentBytesLen;
  using primitives::cid::pieceCommitmentV1ToCID;
  using primitives::cid::replicaCommitmentV1ToCID;
  using primitives::sector::getRegisteredWindowPoStProof;
  using primitives::sector::getRegisteredWinningPoStProof;
  using primitives::sector::RegisteredAggregationProof;
  using sector_storage::zerocomm::getZeroPieceCommitment;

  outcome::result<WriteWithoutAlignmentResult> cppWriteWithoutAlignmentResult(
      const fil_WriteWithoutAlignmentResponse &response) {
    WriteWithoutAlignmentResult result;

    result.total_write_unpadded = response.total_write_unpadded;
    OUTCOME_TRYA(
        result.piece_cid,
        pieceCommitmentV1ToCID(gsl::span(response.comm_p, kCommitmentBytesLen)))

    return result;
  }

  outcome::result<WriteWithAlignmentResult> cppWriteWithAlignmentResult(
      const fil_WriteWithAlignmentResponse &response) {
    WriteWithAlignmentResult result;

    result.left_alignment_unpadded = response.left_alignment_unpadded;
    result.total_write_unpadded = response.total_write_unpadded;
    OUTCOME_TRYA(result.piece_cid,
                 pieceCommitmentV1ToCID(
                     gsl::span(response.comm_p, kCommitmentBytesLen)));

    return result;
  }

  ProofsError cppResponseStatus(FCPResponseStatus status) {
    switch (status) {
      case FCPResponseStatus::FCPResponseStatus_FCPNoError:
        return static_cast<ProofsError>(0);  // success
      case FCPResponseStatus::FCPResponseStatus_FCPUnclassifiedError:
        return ProofsError::kUnclassifiedError;
      case FCPResponseStatus::FCPResponseStatus_FCPCallerError:
        return ProofsError::kCallerError;
      case FCPResponseStatus::FCPResponseStatus_FCPReceiverError:
        return ProofsError::kReceiverError;
      default:
        return ProofsError::kUnknown;
    }
  }

  outcome::result<RegisteredPoStProof> cppRegisteredPoStProof(
      fil_RegisteredPoStProof proof_type) {
    switch (proof_type) {
      case fil_RegisteredPoStProof::
          fil_RegisteredPoStProof_StackedDrgWindow2KiBV1:
        return RegisteredPoStProof::kStackedDRG2KiBWindowPoSt;
      case fil_RegisteredPoStProof::
          fil_RegisteredPoStProof_StackedDrgWindow8MiBV1:
        return RegisteredPoStProof::kStackedDRG8MiBWindowPoSt;
      case fil_RegisteredPoStProof::
          fil_RegisteredPoStProof_StackedDrgWindow512MiBV1:
        return RegisteredPoStProof::kStackedDRG512MiBWindowPoSt;
      case fil_RegisteredPoStProof::
          fil_RegisteredPoStProof_StackedDrgWindow32GiBV1:
        return RegisteredPoStProof::kStackedDRG32GiBWindowPoSt;
      case fil_RegisteredPoStProof::
          fil_RegisteredPoStProof_StackedDrgWindow64GiBV1:
        return RegisteredPoStProof::kStackedDRG64GiBWindowPoSt;
      case fil_RegisteredPoStProof::
          fil_RegisteredPoStProof_StackedDrgWinning2KiBV1:
        return RegisteredPoStProof::kStackedDRG2KiBWinningPoSt;
      case fil_RegisteredPoStProof::
          fil_RegisteredPoStProof_StackedDrgWinning8MiBV1:
        return RegisteredPoStProof::kStackedDRG8MiBWinningPoSt;
      case fil_RegisteredPoStProof::
          fil_RegisteredPoStProof_StackedDrgWinning512MiBV1:
        return RegisteredPoStProof::kStackedDRG512MiBWinningPoSt;
      case fil_RegisteredPoStProof::
          fil_RegisteredPoStProof_StackedDrgWinning32GiBV1:
        return RegisteredPoStProof::kStackedDRG32GiBWinningPoSt;
      case fil_RegisteredPoStProof::
          fil_RegisteredPoStProof_StackedDrgWinning64GiBV1:
        return RegisteredPoStProof::kStackedDRG64GiBWinningPoSt;
      default:
        return ProofsError::kInvalidPostProof;
    }
  }

  outcome::result<PoStProof> cppPoStProof(const fil_PoStProof &c_post_proof) {
    OUTCOME_TRY(cpp_registered_proof,
                cppRegisteredPoStProof(c_post_proof.registered_proof));
    return PoStProof{cpp_registered_proof,
                     {c_post_proof.proof_ptr,
                      c_post_proof.proof_ptr + c_post_proof.proof_len}};
  }

  outcome::result<std::vector<PoStProof>> cppPoStProofs(
      gsl::span<const fil_PoStProof> c_post_proofs) {
    std::vector<PoStProof> cpp_post_proofs = {};

    for (const auto &c_post_proof : c_post_proofs) {
      OUTCOME_TRY(cpp_post_proof, cppPoStProof(c_post_proof));
      cpp_post_proofs.push_back(cpp_post_proof);
    }

    return cpp_post_proofs;
  }

  outcome::result<fil_RegisteredPoStProof> cRegisteredPoStProof(
      RegisteredPoStProof proof_type) {
    switch (proof_type) {
      case RegisteredPoStProof::kStackedDRG2KiBWindowPoSt:
        return fil_RegisteredPoStProof::
            fil_RegisteredPoStProof_StackedDrgWindow2KiBV1;
      case RegisteredPoStProof::kStackedDRG8MiBWindowPoSt:
        return fil_RegisteredPoStProof::
            fil_RegisteredPoStProof_StackedDrgWindow8MiBV1;
      case RegisteredPoStProof::kStackedDRG512MiBWindowPoSt:
        return fil_RegisteredPoStProof::
            fil_RegisteredPoStProof_StackedDrgWindow512MiBV1;
      case RegisteredPoStProof::kStackedDRG32GiBWindowPoSt:
        return fil_RegisteredPoStProof::
            fil_RegisteredPoStProof_StackedDrgWindow32GiBV1;
      case RegisteredPoStProof::kStackedDRG64GiBWindowPoSt:
        return fil_RegisteredPoStProof::
            fil_RegisteredPoStProof_StackedDrgWindow64GiBV1;

      case RegisteredPoStProof::kStackedDRG2KiBWinningPoSt:
        return fil_RegisteredPoStProof::
            fil_RegisteredPoStProof_StackedDrgWinning2KiBV1;
      case RegisteredPoStProof::kStackedDRG8MiBWinningPoSt:
        return fil_RegisteredPoStProof::
            fil_RegisteredPoStProof_StackedDrgWinning8MiBV1;
      case RegisteredPoStProof::kStackedDRG512MiBWinningPoSt:
        return fil_RegisteredPoStProof::
            fil_RegisteredPoStProof_StackedDrgWinning512MiBV1;
      case RegisteredPoStProof::kStackedDRG32GiBWinningPoSt:
        return fil_RegisteredPoStProof::
            fil_RegisteredPoStProof_StackedDrgWinning32GiBV1;
      case RegisteredPoStProof::kStackedDRG64GiBWinningPoSt:
        return fil_RegisteredPoStProof::
            fil_RegisteredPoStProof_StackedDrgWinning64GiBV1;
      default:
        return ProofsError::kNoSuchPostProof;
    }
  }

  enum PoStType {
    Window,
    Winning,
    Either,
  };

  outcome::result<fil_RegisteredPoStProof> cRegisteredPoStProof(
      RegisteredSealProof proof_type, PoStType post_type) {
    RegisteredPoStProof proof;
    switch (post_type) {
      case PoStType::Window: {
        OUTCOME_TRYA(proof, getRegisteredWindowPoStProof(proof_type));
        break;
      }
      case PoStType::Winning: {
        OUTCOME_TRYA(proof, getRegisteredWinningPoStProof(proof_type));
        break;
      }
      case PoStType::Either: {
        auto proof_opt = getRegisteredWinningPoStProof(proof_type);
        if (proof_opt.has_error()) {
          OUTCOME_TRYA(proof, getRegisteredWindowPoStProof(proof_type));
        } else {
          proof = proof_opt.value();
        }
        break;
      }
      default:
        return ProofsError::kNoSuchPostProof;
    }

    return cRegisteredPoStProof(proof);
  }

  outcome::result<fil_RegisteredSealProof> cRegisteredSealProof(
      RegisteredSealProof proof_type) {
    switch (proof_type) {
      case RegisteredSealProof::kStackedDrg2KiBV1:
        return fil_RegisteredSealProof::
            fil_RegisteredSealProof_StackedDrg2KiBV1;
      case RegisteredSealProof::kStackedDrg8MiBV1:
        return fil_RegisteredSealProof::
            fil_RegisteredSealProof_StackedDrg8MiBV1;
      case RegisteredSealProof::kStackedDrg512MiBV1:
        return fil_RegisteredSealProof::
            fil_RegisteredSealProof_StackedDrg512MiBV1;
      case RegisteredSealProof::kStackedDrg32GiBV1:
        return fil_RegisteredSealProof::
            fil_RegisteredSealProof_StackedDrg32GiBV1;
      case RegisteredSealProof::kStackedDrg64GiBV1:
        return fil_RegisteredSealProof::
            fil_RegisteredSealProof_StackedDrg64GiBV1;
      case RegisteredSealProof::kStackedDrg2KiBV1_1:
        return fil_RegisteredSealProof::
            fil_RegisteredSealProof_StackedDrg2KiBV1_1;
      case RegisteredSealProof::kStackedDrg8MiBV1_1:
        return fil_RegisteredSealProof::
            fil_RegisteredSealProof_StackedDrg8MiBV1_1;
      case RegisteredSealProof::kStackedDrg512MiBV1_1:
        return fil_RegisteredSealProof::
            fil_RegisteredSealProof_StackedDrg512MiBV1_1;
      case RegisteredSealProof::kStackedDrg32GiBV1_1:
        return fil_RegisteredSealProof::
            fil_RegisteredSealProof_StackedDrg32GiBV1_1;
      case RegisteredSealProof::kStackedDrg64GiBV1_1:
        return fil_RegisteredSealProof::
            fil_RegisteredSealProof_StackedDrg64GiBV1_1;
      default:
        return ProofsError::kNoSuchSealProof;
    }
  }

  outcome::result<fil_RegisteredAggregationProof> cRegisteredAggregationProof(
      RegisteredAggregationProof type) {
    switch (type) {
      case RegisteredAggregationProof::SnarkPackV1:
        return fil_RegisteredAggregationProof_SnarkPackV1;
      default:
        return ProofsError::kNoSuchAggregationSealProof;
    }
  }

  fil_32ByteArray toProverID(ActorId miner_id) {
    fil_32ByteArray prover = {};
    const codec::uvarint::VarintEncoder _maddr{miner_id};
    const auto maddr{_maddr.bytes()};
    std::copy(maddr.begin(), maddr.end(), prover.inner);
    return prover;
  }

  inline auto &c32ByteArray(const Hash256 &arr) {
    return (const fil_32ByteArray &)arr;
  }

  outcome::result<fil_PublicReplicaInfo> cPublicReplicaInfo(
      const SectorInfo &cpp_public_replica_info, PoStType post_type) {
    fil_PublicReplicaInfo c_public_replica_info{};

    c_public_replica_info.sector_id = cpp_public_replica_info.sector;

    OUTCOME_TRY(c_proof_type,
                cRegisteredPoStProof(cpp_public_replica_info.registered_proof,
                                     post_type));

    c_public_replica_info.registered_proof = c_proof_type;

    OUTCOME_TRY(comm_r,
                CIDToReplicaCommitmentV1(cpp_public_replica_info.sealed_cid));
    ffi::array(c_public_replica_info.comm_r, comm_r);

    return c_public_replica_info;
  }

  outcome::result<std::vector<fil_PublicReplicaInfo>> cPublicReplicaInfos(
      gsl::span<const SectorInfo> cpp_public_replicas_info,
      PoStType post_type) {
    std::vector<fil_PublicReplicaInfo> c_public_replicas_info = {};
    for (const auto &cpp_public_replica_info : cpp_public_replicas_info) {
      OUTCOME_TRY(c_public_replica_info,
                  cPublicReplicaInfo(cpp_public_replica_info, post_type));
      c_public_replicas_info.push_back(c_public_replica_info);
    }
    return c_public_replicas_info;
  }

  outcome::result<fil_PublicPieceInfo> cPublicPieceInfo(
      const PieceInfo &cpp_public_piece_info) {
    fil_PublicPieceInfo c_public_piece_info{};

    c_public_piece_info.num_bytes = cpp_public_piece_info.size.unpadded();
    OUTCOME_TRY(comm_p, CIDToPieceCommitmentV1(cpp_public_piece_info.cid));
    ffi::array(c_public_piece_info.comm_p, comm_p);

    return c_public_piece_info;
  }

  outcome::result<std::vector<fil_PublicPieceInfo>> cPublicPieceInfos(
      gsl::span<const PieceInfo> cpp_public_pieces_info) {
    std::vector<fil_PublicPieceInfo> c_public_pieces_info = {};
    for (const auto &cpp_public_piece_info : cpp_public_pieces_info) {
      OUTCOME_TRY(c_piblic_piece_info, cPublicPieceInfo(cpp_public_piece_info));
      c_public_pieces_info.push_back(c_piblic_piece_info);
    }
    return c_public_pieces_info;
  }

  outcome::result<fil_PrivateReplicaInfo> cPrivateReplicaInfo(
      const PrivateSectorInfo &cpp_private_replica_info, PoStType post_type) {
    fil_PrivateReplicaInfo c_private_replica_info;

    c_private_replica_info.sector_id = cpp_private_replica_info.info.sector;

    c_private_replica_info.cache_dir_path =
        cpp_private_replica_info.cache_dir_path.data();
    c_private_replica_info.replica_path =
        cpp_private_replica_info.sealed_sector_path.data();
    OUTCOME_TRY(c_proof_type,
                cRegisteredPoStProof(cpp_private_replica_info.post_proof_type));

    c_private_replica_info.registered_proof = c_proof_type;

    OUTCOME_TRY(
        comm_r,
        CIDToReplicaCommitmentV1(cpp_private_replica_info.info.sealed_cid));
    ffi::array(c_private_replica_info.comm_r, comm_r);

    return c_private_replica_info;
  }

  outcome::result<std::vector<fil_PrivateReplicaInfo>> cPrivateReplicasInfo(
      gsl::span<const PrivateSectorInfo> cpp_private_replicas_info,
      PoStType post_type) {
    std::vector<fil_PrivateReplicaInfo> c_private_replicas_info = {};
    for (const auto &cpp_private_replica_info : cpp_private_replicas_info) {
      OUTCOME_TRY(c_private_replica_info,
                  cPrivateReplicaInfo(cpp_private_replica_info, post_type));
      c_private_replicas_info.push_back(c_private_replica_info);
    }
    return c_private_replicas_info;
  }

  outcome::result<fil_PoStProof> cPoStProof(const PoStProof &cpp_post_proof) {
    OUTCOME_TRY(c_proof, cRegisteredPoStProof(cpp_post_proof.registered_proof));
    return fil_PoStProof{
        .registered_proof = c_proof,
        .proof_len = cpp_post_proof.proof.size(),
        .proof_ptr = cpp_post_proof.proof.data(),
    };
  }

  outcome::result<std::vector<fil_PoStProof>> cPoStProofs(
      gsl::span<const PoStProof> cpp_post_proofs) {
    std::vector<fil_PoStProof> c_proofs = {};
    for (const auto &cpp_post_proof : cpp_post_proofs) {
      OUTCOME_TRY(c_post_proof, cPoStProof(cpp_post_proof));
      c_proofs.push_back(c_post_proof);
    }
    return c_proofs;
  }

  outcome::result<WriteWithoutAlignmentResult>
  ProofEngineImpl::writeWithoutAlignment(
      RegisteredSealProof proof_type,
      const PieceData &piece_data,
      const UnpaddedPieceSize &piece_bytes,
      const std::string &staged_sector_file_path) {
    OUTCOME_TRY(c_proof_type, cRegisteredSealProof(proof_type));

    if (!piece_data.isOpened()) {
      return ProofsError::kCannotOpenFile;
    }
    int staged_sector_fd;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
    if ((staged_sector_fd =
             open(staged_sector_file_path.c_str(), O_RDWR | O_CREAT, 0644))
        == -1) {
      return ProofsError::kCannotOpenFile;
    }

    auto res_ptr = ffi::wrap(
        fil_write_without_alignment(
            c_proof_type, piece_data.getFd(), piece_bytes, staged_sector_fd),
        fil_destroy_write_without_alignment_response);

    // NOLINTNEXTLINE(readability-implicit-bool-conversion)
    if (close(staged_sector_fd))
      logger_->warn("writeWithoutAlignment: error in closing file "
                    + staged_sector_file_path);
    PROOFS_TRY("writeWithoutAlignment");
    return cppWriteWithoutAlignmentResult(*res_ptr);
  }

  outcome::result<WriteWithAlignmentResult> ProofEngineImpl::writeWithAlignment(
      RegisteredSealProof proof_type,
      const PieceData &piece_data,
      const UnpaddedPieceSize &piece_bytes,
      const std::string &staged_sector_file_path,
      gsl::span<const UnpaddedPieceSize> existing_piece_sizes) {
    OUTCOME_TRY(c_proof_type, cRegisteredSealProof(proof_type));

    if (!piece_data.isOpened()) {
      return ProofsError::kCannotOpenFile;
    }

    int staged_sector_fd;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
    if ((staged_sector_fd = open(staged_sector_file_path.c_str(), O_RDWR, 0644))
        == -1) {
      return ProofsError::kCannotOpenFile;
    }

    UnpaddedPieceSize offset(0);

    for (const auto &piece_size : existing_piece_sizes) {
      offset += piece_size;
    }

    if (lseek(staged_sector_fd, offset.padded(), SEEK_SET) == -1) {
      // NOLINTNEXTLINE(readability-implicit-bool-conversion)
      if (close(staged_sector_fd))
        logger_->warn("writeWithAlignment: error in closing file "
                      + staged_sector_file_path);
      return ProofsError::kUnableMoveCursor;
    }

    auto res_ptr = ffi::wrap(fil_write_with_alignment(c_proof_type,
                                                      piece_data.getFd(),
                                                      uint64_t(piece_bytes),
                                                      staged_sector_fd,
                                                      nullptr,
                                                      0),
                             fil_destroy_write_with_alignment_response);

    // NOLINTNEXTLINE(readability-implicit-bool-conversion)
    if (close(staged_sector_fd))
      logger_->warn("writeWithAlignment: error in closing file "
                    + staged_sector_file_path);
    PROOFS_TRY("writeWithAlignment");
    return cppWriteWithAlignmentResult(*res_ptr);
  }

  outcome::result<void> ProofEngineImpl::readPiece(
      PieceData output,
      const std::string &unsealed_file,
      const PaddedPieceSize &offset,
      const UnpaddedPieceSize &piece_size) {
    if (!output.isOpened()) {
      return ProofsError::kCannotOpenFile;
    }

    OUTCOME_TRY(piece_size.validate());

    if (!fs::exists(unsealed_file)) {
      return ProofsError::kFileDoesntExist;
    }

    auto max_size = fs::file_size(unsealed_file);

    if ((offset + piece_size.padded()) > max_size) {
      return ProofsError::kOutOfBound;
    }

    std::ifstream input(unsealed_file);
    if (!input.good()) {
      return ProofsError::kCannotOpenFile;
    }
    if (!input.seekg(offset, std::ios_base::beg)) {
      return ProofsError::kUnableMoveCursor;
    }

    uint64_t left = piece_size;
    constexpr auto kDefaultBufferSize = uint64_t(32 * 1024);
    auto chunks = kDefaultBufferSize / 127;
    PaddedPieceSize outTwoPow =
        primitives::piece::paddedSize(chunks * 128).padded();
    std::vector<uint8_t> buffer(outTwoPow.unpadded());

    while (left > 0) {
      if (left < outTwoPow.unpadded()) {
        outTwoPow = primitives::piece::paddedSize(left).padded();
      }
      std::vector<uint8_t> read(outTwoPow);

      size_t j{};
      char ch;
      for (j = 0; j < outTwoPow && input; j++) {
        input.get(ch);
        read[j] = ch;
      }

      if (j != outTwoPow) {
        return ProofsError::kNotReadEnough;
      }

      primitives::piece::unpad(
          gsl::make_span(read.data(), outTwoPow),
          gsl::make_span(buffer.data(), outTwoPow.unpadded()));

      uint64_t write_size =
          write(output.getFd(), buffer.data(), outTwoPow.unpadded());

      if (write_size != outTwoPow.unpadded()) {
        return ProofsError::kNotWriteEnough;
      }

      left -= outTwoPow.unpadded();
    }
    return outcome::success();
  }

  outcome::result<Phase1Output> ProofEngineImpl::sealPreCommitPhase1(
      RegisteredSealProof proof_type,
      const std::string &cache_dir_path,
      const std::string &staged_sector_path,
      const std::string &sealed_sector_path,
      SectorNumber sector_num,
      ActorId miner_id,
      const SealRandomness &ticket,
      gsl::span<const PieceInfo> pieces) {
    OUTCOME_TRY(c_proof_type, cRegisteredSealProof(proof_type));

    OUTCOME_TRY(c_pieces, cPublicPieceInfos(pieces));

    auto prover_id = toProverID(miner_id);

    auto res_ptr =
        ffi::wrap(fil_seal_pre_commit_phase1(c_proof_type,
                                             cache_dir_path.c_str(),
                                             staged_sector_path.c_str(),
                                             sealed_sector_path.c_str(),
                                             sector_num,
                                             prover_id,
                                             c32ByteArray(ticket),
                                             c_pieces.data(),
                                             c_pieces.size()),
                  fil_destroy_seal_pre_commit_phase1_response);
    PROOFS_TRY("sealPreCommitPhase1");
    return Phase1Output(
        res_ptr->seal_pre_commit_phase1_output_ptr,
        res_ptr->seal_pre_commit_phase1_output_ptr
            + res_ptr->seal_pre_commit_phase1_output_len);  // NOLINT
  }

  outcome::result<SealedAndUnsealedCID> ProofEngineImpl::sealPreCommitPhase2(
      gsl::span<const uint8_t> phase1_output,
      const std::string &cache_dir_path,
      const std::string &sealed_sector_path) {
    auto res_ptr =
        ffi::wrap(fil_seal_pre_commit_phase2(phase1_output.data(),
                                             phase1_output.size(),
                                             cache_dir_path.c_str(),
                                             sealed_sector_path.c_str()),
                  fil_destroy_seal_pre_commit_phase2_response);
    PROOFS_TRY("sealPreCommitPhase2");
    SealedAndUnsealedCID result;

    OUTCOME_TRYA(result.sealed_cid,
                 replicaCommitmentV1ToCID(
                     gsl::make_span(res_ptr->comm_r, kCommitmentBytesLen)));

    OUTCOME_TRYA(result.unsealed_cid,
                 dataCommitmentV1ToCID(
                     gsl::make_span(res_ptr->comm_d, kCommitmentBytesLen)));

    return result;
  }

  outcome::result<Phase1Output> ProofEngineImpl::sealCommitPhase1(
      RegisteredSealProof proof_type,
      const CID &sealed_cid,
      const CID &unsealed_cid,
      const std::string &cache_dir_path,
      const std::string &sealed_sector_path,
      SectorNumber sector_num,
      ActorId miner_id,
      const Ticket &ticket,
      const Seed &seed,
      gsl::span<const PieceInfo> pieces) {
    OUTCOME_TRY(c_proof_type, cRegisteredSealProof(proof_type));

    OUTCOME_TRY(c_pieces, cPublicPieceInfos(pieces));

    OUTCOME_TRY(comm_r, CIDToReplicaCommitmentV1(sealed_cid));

    OUTCOME_TRY(comm_d, CIDToDataCommitmentV1(unsealed_cid));

    auto prover_id = toProverID(miner_id);

    auto res_ptr = ffi::wrap(fil_seal_commit_phase1(c_proof_type,
                                                    c32ByteArray(comm_r),
                                                    c32ByteArray(comm_d),
                                                    cache_dir_path.c_str(),
                                                    sealed_sector_path.c_str(),
                                                    sector_num,
                                                    prover_id,
                                                    c32ByteArray(ticket),
                                                    c32ByteArray(seed),
                                                    c_pieces.data(),
                                                    c_pieces.size()),
                             fil_destroy_seal_commit_phase1_response);
    PROOFS_TRY("sealCommitPhase1");
    return Phase1Output(
        res_ptr->seal_commit_phase1_output_ptr,
        res_ptr->seal_commit_phase1_output_ptr
            + res_ptr->seal_commit_phase1_output_len);  // NOLINT
  }

  outcome::result<Proof> ProofEngineImpl::sealCommitPhase2(
      gsl::span<const uint8_t> phase1_output,
      SectorNumber sector_id,
      ActorId miner_id) {
    auto prover_id = toProverID(miner_id);
    auto res_ptr = ffi::wrap(
        fil_seal_commit_phase2(
            phase1_output.data(), phase1_output.size(), sector_id, prover_id),
        fil_destroy_seal_commit_phase2_response);
    PROOFS_TRY("sealCommitPhase2");
    return Proof(res_ptr->proof_ptr, res_ptr->proof_ptr + res_ptr->proof_len);
  }

  outcome::result<CID> ProofEngineImpl::generatePieceCIDFromFile(
      RegisteredSealProof proof_type,
      const std::string &piece_file_path,
      UnpaddedPieceSize piece_size) {
    return generatePieceCID(proof_type, PieceData(piece_file_path), piece_size);
  }

  outcome::result<CID> ProofEngineImpl::generatePieceCID(
      RegisteredSealProof proof_type, gsl::span<const uint8_t> data) {
    assert(UnpaddedPieceSize(data.size()).validate());

    int fds[2];
    if (pipe(fds) < 0) {
      return ProofsError::kCannotCreatePipe;
    }

    std::error_code ec;
    std::thread t([output = PieceData(fds[1]), &data, &ec]() {
      if (write(output.getFd(), (char *)data.data(), data.size()) == -1) {
        ec = ProofsError::kCannotWriteData;
      }
    });

    auto maybe_cid = generatePieceCID(
        proof_type, PieceData(fds[0]), UnpaddedPieceSize(data.size()));

    t.join();

    if (ec) {
      return ec;
    }

    return maybe_cid;
  }

  outcome::result<CID> ProofEngineImpl::generatePieceCID(
      RegisteredSealProof proof_type,
      const PieceData &piece,
      UnpaddedPieceSize piece_size) {
    OUTCOME_TRY(c_proof_type, cRegisteredSealProof(proof_type));

    if (!piece.isOpened()) {
      return ProofsError::kCannotOpenFile;
    }

    auto res_ptr = ffi::wrap(
        fil_generate_piece_commitment(c_proof_type, piece.getFd(), piece_size),
        fil_destroy_generate_piece_commitment_response);
    PROOFS_TRY("generatePieceCID");
    return dataCommitmentV1ToCID(
        gsl::make_span(res_ptr->comm_p, kCommitmentBytesLen));
  }

  outcome::result<CID> ProofEngineImpl::generateUnsealedCID(
      RegisteredSealProof proof_type,
      gsl::span<const PieceInfo> pieces,
      bool pad) {
    std::vector<PieceInfo> padded;
    if (pad) {
      OUTCOME_TRY(_sector, primitives::sector::getSectorSize(proof_type));
      PaddedPieceSize sector{_sector};
      if (pieces.empty()) {
        OUTCOME_TRY(zero, getZeroPieceCommitment(sector.unpadded()));
        padded.push_back({sector, zero});
      } else {
        PaddedPieceSize sum;
        auto pad{[&](auto size) -> outcome::result<void> {
          auto padding{getRequiredPadding(sum, size)};
          for (auto pad : padding.pads) {
            OUTCOME_TRY(zero, getZeroPieceCommitment(pad.unpadded()));
            padded.push_back({pad, zero});
          }
          sum += padding.size;
          return outcome::success();
        }};
        for (auto &piece : pieces) {
          OUTCOME_TRY(pad(piece.size));
          padded.push_back(piece);
          sum += piece.size;
        }
        OUTCOME_TRY(pad(sector));
      }
      pieces = padded;
    }

    OUTCOME_TRY(c_proof_type, cRegisteredSealProof(proof_type));

    OUTCOME_TRY(c_pieces, cPublicPieceInfos(pieces));

    auto res_ptr =
        ffi::wrap(fil_generate_data_commitment(
                      c_proof_type, c_pieces.data(), c_pieces.size()),
                  fil_destroy_generate_data_commitment_response);
    PROOFS_TRY("generateUnsealedCID");
    return dataCommitmentV1ToCID(
        gsl::make_span(res_ptr->comm_d, kCommitmentBytesLen));
  }

  outcome::result<ChallengeIndexes>
  ProofEngineImpl::generateWinningPoStSectorChallenge(
      RegisteredPoStProof proof_type,
      ActorId miner_id,
      const PoStRandomness &randomness,
      uint64_t eligible_sectors_len) {
    auto rand31{randomness};
    rand31[31] = 0;

    OUTCOME_TRY(c_proof_type, cRegisteredPoStProof(proof_type));
    auto prover_id = toProverID(miner_id);

    auto res_ptr = ffi::wrap(
        fil_generate_winning_post_sector_challenge(c_proof_type,
                                                   c32ByteArray(rand31),
                                                   eligible_sectors_len,
                                                   prover_id),
        fil_destroy_generate_winning_post_sector_challenge);
    PROOFS_TRY("generateWinningPoStSectorChallenge");
    return ChallengeIndexes(res_ptr->ids_ptr,
                            res_ptr->ids_ptr + res_ptr->ids_len);  // NOLINT
  }

  outcome::result<std::vector<PoStProof>> ProofEngineImpl::generateWinningPoSt(
      ActorId miner_id,
      const SortedPrivateSectorInfo &private_replica_info,
      const PoStRandomness &randomness) {
    OUTCOME_TRY(
        c_sorted_private_sector_info,
        cPrivateReplicasInfo(private_replica_info.values, PoStType::Winning));

    auto prover_id = toProverID(miner_id);
    auto res_ptr =
        ffi::wrap(fil_generate_winning_post(c32ByteArray(randomness),
                                            c_sorted_private_sector_info.data(),
                                            c_sorted_private_sector_info.size(),
                                            prover_id),
                  fil_destroy_generate_winning_post_response);
    PROOFS_TRY("generateWinningPoSt");
    return cppPoStProofs(
        gsl::make_span(res_ptr->proofs_ptr, res_ptr->proofs_len));
  }

  outcome::result<std::vector<PoStProof>> ProofEngineImpl::generateWindowPoSt(
      ActorId miner_id,
      const SortedPrivateSectorInfo &private_replica_info,
      const PoStRandomness &randomness) {
    OUTCOME_TRY(
        c_sorted_private_sector_info,
        cPrivateReplicasInfo(private_replica_info.values, PoStType::Window));

    auto prover_id = toProverID(miner_id);
    auto res_ptr =
        ffi::wrap(fil_generate_window_post(c32ByteArray(randomness),
                                           c_sorted_private_sector_info.data(),
                                           c_sorted_private_sector_info.size(),
                                           prover_id),
                  fil_destroy_generate_window_post_response);
    PROOFS_TRY("generateWindowPoSt");
    return cppPoStProofs(
        gsl::make_span(res_ptr->proofs_ptr, res_ptr->proofs_len));
  }

  outcome::result<bool> ProofEngineImpl::verifyWinningPoSt(
      const WinningPoStVerifyInfo &info) {
    OUTCOME_TRY(
        c_public_replica_infos,
        cPublicReplicaInfos(info.challenged_sectors, PoStType::Winning));
    OUTCOME_TRY(c_post_proofs, cPoStProofs(gsl::make_span(info.proofs)));
    auto prover_id = toProverID(info.prover);

    auto res_ptr =
        ffi::wrap(fil_verify_winning_post(c32ByteArray(info.randomness),
                                          c_public_replica_infos.data(),
                                          c_public_replica_infos.size(),
                                          c_post_proofs.data(),
                                          c_post_proofs.size(),
                                          prover_id),
                  fil_destroy_verify_winning_post_response);
    PROOFS_TRY("verifyWinningPoSt");
    return res_ptr->is_valid;
  }

  outcome::result<bool> ProofEngineImpl::verifyWindowPoSt(
      const WindowPoStVerifyInfo &info) {
    OUTCOME_TRY(c_public_replica_infos,
                cPublicReplicaInfos(info.challenged_sectors, PoStType::Window));
    OUTCOME_TRY(c_post_proofs, cPoStProofs(gsl::make_span(info.proofs)));
    auto prover_id = toProverID(info.prover);

    auto res_ptr =
        ffi::wrap(fil_verify_window_post(c32ByteArray(info.randomness),
                                         c_public_replica_infos.data(),
                                         c_public_replica_infos.size(),
                                         c_post_proofs.data(),
                                         c_post_proofs.size(),
                                         prover_id),
                  fil_destroy_verify_window_post_response);
    PROOFS_TRY("verifyWindowPoSt");
    return res_ptr->is_valid;
  }

  outcome::result<bool> ProofEngineImpl::verifySeal(
      const SealVerifyInfo &info) {
    OUTCOME_TRY(c_proof_type, cRegisteredSealProof(info.seal_proof));

    OUTCOME_TRY(comm_r, CIDToReplicaCommitmentV1(info.sealed_cid));

    OUTCOME_TRY(comm_d, CIDToDataCommitmentV1(info.unsealed_cid));

    auto prover_id = toProverID(info.sector.miner);

    auto res_ptr =
        ffi::wrap(fil_verify_seal(c_proof_type,
                                  c32ByteArray(comm_r),
                                  c32ByteArray(comm_d),
                                  prover_id,
                                  c32ByteArray(info.randomness),
                                  c32ByteArray(info.interactive_randomness),
                                  info.sector.sector,
                                  info.proof.data(),
                                  info.proof.size()),
                  fil_destroy_verify_seal_response);
    PROOFS_TRY("verifySeal");
    return res_ptr->is_valid;
  }

  outcome::result<void> ProofEngineImpl::aggregateSealProofs(
      AggregateSealVerifyProofAndInfos &aggregate,
      const std::vector<BytesIn> &proofs) {
    OUTCOME_TRY(c_seal_proof, cRegisteredSealProof(aggregate.seal_proof));
    OUTCOME_TRY(c_aggregate_proof,
                cRegisteredAggregationProof(aggregate.aggregate_proof));
    std::vector<fil_32ByteArray> c_commrs, c_seeds;
    c_commrs.reserve(aggregate.infos.size());
    c_seeds.reserve(aggregate.infos.size());
    for (const auto &info : aggregate.infos) {
      OUTCOME_TRY(comm_r, CIDToReplicaCommitmentV1(info.sealed_cid));
      c_commrs.push_back(c32ByteArray(comm_r));
      c_seeds.push_back(c32ByteArray(info.interactive_randomness));
    }
    std::vector<fil_SealCommitPhase2Response> c_proofs;
    c_proofs.reserve(proofs.size());
    for (const auto &proof : proofs) {
      auto &c_proof{c_proofs.emplace_back()};
      c_proof.proof_ptr = proof.data();
      c_proof.proof_len = proof.size();
    }
    const auto res_ptr{ffi::wrap(fil_aggregate_seal_proofs(c_seal_proof,
                                                           c_aggregate_proof,
                                                           c_commrs.data(),
                                                           c_commrs.size(),
                                                           c_seeds.data(),
                                                           c_seeds.size(),
                                                           c_proofs.data(),
                                                           c_proofs.size()),
                                 fil_destroy_aggregate_proof)};
    PROOFS_TRY("aggregateSealProofs");
    copy(aggregate.proof, BytesIn(res_ptr->proof_ptr, res_ptr->proof_len));
    return outcome::success();
  }

  outcome::result<bool> ProofEngineImpl::verifyAggregateSeals(
      const AggregateSealVerifyProofAndInfos &aggregate) {
    OUTCOME_TRY(c_seal_proof, cRegisteredSealProof(aggregate.seal_proof));
    OUTCOME_TRY(c_aggregate_proof,
                cRegisteredAggregationProof(aggregate.aggregate_proof));
    const auto prover_id{toProverID(aggregate.miner)};
    std::vector<fil_AggregationInputs> c_infos;
    c_infos.reserve(aggregate.infos.size());
    for (const auto &info : aggregate.infos) {
      OUTCOME_TRY(comm_r, CIDToReplicaCommitmentV1(info.sealed_cid));
      OUTCOME_TRY(comm_d, CIDToDataCommitmentV1(info.unsealed_cid));
      c_infos.push_back(fil_AggregationInputs{
          c32ByteArray(comm_r),
          c32ByteArray(comm_d),
          info.number,
          c32ByteArray(info.randomness),
          c32ByteArray(info.interactive_randomness),
      });
    }
    const auto res_ptr{
        ffi::wrap(fil_verify_aggregate_seal_proof(c_seal_proof,
                                                  c_aggregate_proof,
                                                  prover_id,
                                                  aggregate.proof.data(),
                                                  aggregate.proof.size(),
                                                  c_infos.data(),
                                                  c_infos.size()),
                  fil_destroy_verify_aggregate_seal_response)};
    PROOFS_TRY("verifyAggregateSeals");
    return res_ptr->is_valid;
  }

  outcome::result<void> ProofEngineImpl::unseal(
      RegisteredSealProof proof_type,
      const std::string &cache_dir_path,
      const std::string &sealed_sector_path,
      const std::string &unseal_output_path,
      SectorNumber sector_num,
      ActorId miner_id,
      const Ticket &ticket,
      const UnsealedCID &unsealed_cid) {
    OUTCOME_TRY(size, primitives::sector::getSectorSize(proof_type));
    return unsealRange(proof_type,
                       cache_dir_path,
                       sealed_sector_path,
                       unseal_output_path,
                       sector_num,
                       miner_id,
                       ticket,
                       unsealed_cid,
                       0,
                       PaddedPieceSize{size}.unpadded());
  }

  outcome::result<void> ProofEngineImpl::unsealRange(
      RegisteredSealProof proof_type,
      const std::string &cache_dir_path,
      const PieceData &seal_fd,
      const PieceData &unseal_fd,
      SectorNumber sector_num,
      ActorId miner_id,
      const Ticket &ticket,
      const UnsealedCID &unsealed_cid,
      uint64_t offset,
      uint64_t length) {
    OUTCOME_TRY(c_proof_type, cRegisteredSealProof(proof_type));

    OUTCOME_TRY(comm_d, CIDToDataCommitmentV1(unsealed_cid));

    auto prover_id = toProverID(miner_id);
    auto res_ptr = ffi::wrap(fil_unseal_range(c_proof_type,
                                              cache_dir_path.c_str(),
                                              seal_fd.getFd(),
                                              unseal_fd.getFd(),
                                              sector_num,
                                              prover_id,
                                              c32ByteArray(ticket),
                                              c32ByteArray(comm_d),
                                              offset,
                                              length),
                             fil_destroy_unseal_range_response);
    PROOFS_TRY("unsealRange");
    return outcome::success();
  }

  outcome::result<void> ProofEngineImpl::unsealRange(
      RegisteredSealProof proof_type,
      const std::string &cache_dir_path,
      const std::string &sealed_sector_path,
      const std::string &unseal_output_path,
      SectorNumber sector_num,
      ActorId miner_id,
      const Ticket &ticket,
      const UnsealedCID &unsealed_cid,
      uint64_t offset,
      uint64_t length) {
    PieceData unsealed{unseal_output_path};
    if (!unsealed.isOpened()) {
      return ProofsError::kCannotCreateUnsealedFile;
    }

    PieceData sealed{sealed_sector_path, O_RDONLY};
    if (!sealed.isOpened()) {
      return ProofsError::kCannotOpenFile;
    }

    return unsealRange(proof_type,
                       cache_dir_path,
                       sealed,
                       unsealed,
                       sector_num,
                       miner_id,
                       ticket,
                       unsealed_cid,
                       offset,
                       length);
  }

  outcome::result<void> ProofEngineImpl::clearCache(
      SectorSize sector_size, const std::string &cache_dir_path) {
    auto res_ptr =
        ffi::wrap(fil_clear_cache(sector_size, cache_dir_path.c_str()),
                  fil_destroy_clear_cache_response);
    PROOFS_TRY("clearCache");
    return outcome::success();
  }

  outcome::result<std::string> ProofEngineImpl::getPoStVersion(
      RegisteredPoStProof proof_type) {
    OUTCOME_TRY(c_proof_type, cRegisteredPoStProof(proof_type));
    auto res_ptr = ffi::wrap(fil_get_post_version(c_proof_type),
                             fil_destroy_string_response);
    PROOFS_TRY("getPoStVersion");
    return std::string(res_ptr->string_val);
  }

  outcome::result<std::string> ProofEngineImpl::getSealVersion(
      RegisteredSealProof proof_type) {
    OUTCOME_TRY(c_proof_type, cRegisteredSealProof(proof_type));
    auto res_ptr = ffi::wrap(fil_get_seal_version(c_proof_type),
                             fil_destroy_string_response);
    PROOFS_TRY("getSealVersion");
    return std::string(res_ptr->string_val);
  }

  outcome::result<Devices> ProofEngineImpl::getGPUDevices() {
    auto res_ptr =
        ffi::wrap(fil_get_gpu_devices(), fil_destroy_gpu_device_response);
    PROOFS_TRY("getGPUDevices");
    return Devices(res_ptr->devices_ptr,
                   res_ptr->devices_ptr + res_ptr->devices_len);  // NOLINT
  }

  ProofEngineImpl::ProofEngineImpl()
      : logger_{common::createLogger("proofs")} {}
}  // namespace fc::proofs
