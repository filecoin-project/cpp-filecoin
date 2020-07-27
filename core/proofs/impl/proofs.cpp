/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proofs/proofs.hpp"

#include <fcntl.h>
#include <filecoin-ffi/filcrypto.h>

#include <boost/filesystem.hpp>
#include "common/ffi.hpp"
#include "primitives/address/address.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/cid/comm_cid.hpp"
#include "proofs/proofs_error.hpp"

namespace {
  void unpad(gsl::span<const uint8_t> in, gsl::span<uint8_t> out) {
    auto chunks = in.size() / 128;
    for (auto chunk = 0; chunk < chunks; chunk++) {
      auto input_offset_next = chunk * 128 + 1;
      auto output_offset = chunk * 127;

      auto current = in[chunk * 128];

      for (size_t i = 0; i < 32; i++) {
        out[output_offset + i] = current;

        current = in[i + input_offset_next];
      }

      out[output_offset + 31] |= current << 6;

      for (size_t i = 32; i < 64; i++) {
        auto next = in[i + input_offset_next];

        out[output_offset + i] = current >> 2;
        out[output_offset + i] |= next << 6;

        current = next;
      }

      out[output_offset + 63] ^= (current << 6) ^ (current << 4);

      for (size_t i = 64; i < 96; i++) {
        auto next = in[i + input_offset_next];

        out[output_offset + i] = current >> 4;
        out[output_offset + i] |= next << 4;

        current = next;
      }

      out[output_offset + 95] ^= (current << 4) ^ (current << 2);

      for (size_t i = 96; i < 127; i++) {
        auto next = in[i + input_offset_next];

        out[output_offset + i] = current >> 6;
        out[output_offset + i] |= next << 2;

        current = next;
      }
    }
  }

  void pad(gsl::span<const uint8_t> in, gsl::span<uint8_t> out) {
    auto chunks = out.size() / 128;
    for (auto chunk = 0; chunk < chunks; chunk++) {
      size_t input_offset = chunk * 127;
      size_t output_offset = chunk * 128;

      std::copy(in.begin() + input_offset,
                in.begin() + input_offset + 31,
                out.begin() + output_offset);

      auto t = in[input_offset + 31] >> 6;
      out[output_offset + 31] = in[input_offset + 31] & 0x3f;
      uint8_t v;

      for (int i = 32; i < 64; i++) {
        v = in[input_offset + i];
        out[output_offset + i] = (v << 2) | t;
        t = v >> 6;
      }

      t = v >> 4;
      out[output_offset + 63] &= 0x3f;

      for (size_t i = 64; i < 96; i++) {
        v = in[input_offset + i];
        out[output_offset + i] = (v << 4) | t;
        t = v >> 4;
      }

      t = v >> 2;
      out[output_offset + 95] &= 0x3f;

      for (size_t i = 96; i < 127; i++) {
        v = in[input_offset + i];
        out[output_offset + i] = (v << 6) | t;
        t = v >> 2;
      }

      out[output_offset + 127] = t & 0x3f;
    }
  }
}  // namespace

namespace fc::proofs {
  namespace ffi = common::ffi;
  namespace fs = boost::filesystem;

  common::Logger Proofs::logger_ = common::createLogger("proofs");

  using Prover = Blob<32>;
  using common::Blob;
  using common::Buffer;
  using common::CIDToDataCommitmentV1;
  using common::CIDToPieceCommitmentV1;
  using common::CIDToReplicaCommitmentV1;
  using common::dataCommitmentV1ToCID;
  using common::kCommitmentBytesLen;
  using common::pieceCommitmentV1ToCID;
  using common::replicaCommitmentV1ToCID;
  using crypto::randomness::Randomness;
  using primitives::sector::getRegisteredSealProof;
  using primitives::sector::getRegisteredWindowPoStProof;
  using primitives::sector::getRegisteredWinningPoStProof;
  using primitives::sector::SectorId;

  // ******************
  // TO CPP CASTED FUNCTIONS
  // ******************

  outcome::result<RegisteredProof> cppRegisteredPoStProof(
      fil_RegisteredPoStProof proof_type) {
    switch (proof_type) {
      case fil_RegisteredPoStProof::
          fil_RegisteredPoStProof_StackedDrgWindow2KiBV1:
        return RegisteredProof::StackedDRG2KiBWindowPoSt;
      case fil_RegisteredPoStProof::
          fil_RegisteredPoStProof_StackedDrgWindow8MiBV1:
        return RegisteredProof::StackedDRG8MiBWindowPoSt;
      case fil_RegisteredPoStProof::
          fil_RegisteredPoStProof_StackedDrgWindow512MiBV1:
        return RegisteredProof::StackedDRG512MiBWindowPoSt;
      case fil_RegisteredPoStProof::
          fil_RegisteredPoStProof_StackedDrgWindow32GiBV1:
        return RegisteredProof::StackedDRG32GiBWindowPoSt;
      case fil_RegisteredPoStProof::
          fil_RegisteredPoStProof_StackedDrgWindow64GiBV1:
        return RegisteredProof::StackedDRG64GiBWindowPoSt;
      case fil_RegisteredPoStProof::
          fil_RegisteredPoStProof_StackedDrgWinning2KiBV1:
        return RegisteredProof::StackedDRG2KiBWinningPoSt;
      case fil_RegisteredPoStProof::
          fil_RegisteredPoStProof_StackedDrgWinning8MiBV1:
        return RegisteredProof::StackedDRG8MiBWinningPoSt;
      case fil_RegisteredPoStProof::
          fil_RegisteredPoStProof_StackedDrgWinning512MiBV1:
        return RegisteredProof::StackedDRG512MiBWinningPoSt;
      case fil_RegisteredPoStProof::
          fil_RegisteredPoStProof_StackedDrgWinning32GiBV1:
        return RegisteredProof::StackedDRG32GiBWinningPoSt;
      case fil_RegisteredPoStProof::
          fil_RegisteredPoStProof_StackedDrgWinning64GiBV1:
        return RegisteredProof::StackedDRG64GiBWinningPoSt;
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

  WriteWithoutAlignmentResult cppWriteWithoutAlignmentResult(
      const fil_WriteWithoutAlignmentResponse &response) {
    WriteWithoutAlignmentResult result;

    result.total_write_unpadded = response.total_write_unpadded;
    result.piece_cid =
        pieceCommitmentV1ToCID(gsl::span(response.comm_p, kCommitmentBytesLen));

    return result;
  }

  WriteWithAlignmentResult cppWriteWithAlignmentResult(
      const fil_WriteWithAlignmentResponse &response) {
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

  enum PoStType {
    Window,
    Winning,
    Either,
  };

  outcome::result<fil_RegisteredPoStProof> cRegisteredPoStProof(
      RegisteredProof proof_type, PoStType post_type) {
    RegisteredProof proof;
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

    switch (proof) {
      case RegisteredProof::StackedDRG2KiBWindowPoSt:
        return fil_RegisteredPoStProof::
            fil_RegisteredPoStProof_StackedDrgWindow2KiBV1;
      case RegisteredProof::StackedDRG8MiBWindowPoSt:
        return fil_RegisteredPoStProof::
            fil_RegisteredPoStProof_StackedDrgWindow8MiBV1;
      case RegisteredProof::StackedDRG512MiBWindowPoSt:
        return fil_RegisteredPoStProof::
            fil_RegisteredPoStProof_StackedDrgWindow512MiBV1;
      case RegisteredProof::StackedDRG32GiBWindowPoSt:
        return fil_RegisteredPoStProof::
            fil_RegisteredPoStProof_StackedDrgWindow32GiBV1;
      case RegisteredProof::StackedDRG64GiBWindowPoSt:
        return fil_RegisteredPoStProof::
            fil_RegisteredPoStProof_StackedDrgWindow64GiBV1;

      case RegisteredProof::StackedDRG2KiBWinningPoSt:
        return fil_RegisteredPoStProof::
            fil_RegisteredPoStProof_StackedDrgWinning2KiBV1;
      case RegisteredProof::StackedDRG8MiBWinningPoSt:
        return fil_RegisteredPoStProof::
            fil_RegisteredPoStProof_StackedDrgWinning8MiBV1;
      case RegisteredProof::StackedDRG512MiBWinningPoSt:
        return fil_RegisteredPoStProof::
            fil_RegisteredPoStProof_StackedDrgWinning512MiBV1;
      case RegisteredProof::StackedDRG32GiBWinningPoSt:
        return fil_RegisteredPoStProof::
            fil_RegisteredPoStProof_StackedDrgWinning32GiBV1;
      case RegisteredProof::StackedDRG64GiBWinningPoSt:
        return fil_RegisteredPoStProof::
            fil_RegisteredPoStProof_StackedDrgWinning64GiBV1;
      default:
        return ProofsError::kNoSuchPostProof;
    }
  }

  outcome::result<fil_RegisteredSealProof> cRegisteredSealProof(
      RegisteredProof proof_type) {
    OUTCOME_TRY(seal_proof, getRegisteredSealProof(proof_type));
    switch (seal_proof) {
      case RegisteredProof::StackedDRG2KiBSeal:
        return fil_RegisteredSealProof::
            fil_RegisteredSealProof_StackedDrg2KiBV1;
      case RegisteredProof::StackedDRG8MiBSeal:
        return fil_RegisteredSealProof::
            fil_RegisteredSealProof_StackedDrg8MiBV1;
      case RegisteredProof::StackedDRG512MiBSeal:
        return fil_RegisteredSealProof::
            fil_RegisteredSealProof_StackedDrg512MiBV1;
      case RegisteredProof::StackedDRG32GiBSeal:
        return fil_RegisteredSealProof::
            fil_RegisteredSealProof_StackedDrg32GiBV1;
      case RegisteredProof::StackedDRG64GiBSeal:
        return fil_RegisteredSealProof::
            fil_RegisteredSealProof_StackedDrg64GiBV1;
      default:
        return ProofsError::kNoSuchSealProof;
    }
  }

  fil_32ByteArray toProverID(ActorId miner_id) {
    auto maddr = primitives::address::encode(
        primitives::address::Address::makeFromId(miner_id));
    fil_32ByteArray prover = {};
    // +1: because payload start from 1 position
    std::copy(maddr.cbegin() + 1, maddr.cend(), prover.inner);
    return prover;
  }

  fil_32ByteArray c32ByteArray(const Blob<32> &arr) {
    fil_32ByteArray array{};
    ffi::array(array.inner, arr);
    return array;
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
                cRegisteredPoStProof(cpp_private_replica_info.post_proof_type,
                                     post_type));

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

  outcome::result<fil_PoStProof> cPoStProof(const PoStProof &cpp_post_proof,
                                            PoStType post_type) {
    OUTCOME_TRY(
        c_proof,
        cRegisteredPoStProof(cpp_post_proof.registered_proof, post_type));
    return fil_PoStProof{
        .registered_proof = c_proof,
        .proof_len = cpp_post_proof.proof.size(),
        .proof_ptr = cpp_post_proof.proof.data(),
    };
  }

  outcome::result<std::vector<fil_PoStProof>> cPoStProofs(
      gsl::span<const PoStProof> cpp_post_proofs, PoStType post_type) {
    std::vector<fil_PoStProof> c_proofs = {};
    for (const auto &cpp_post_proof : cpp_post_proofs) {
      OUTCOME_TRY(c_post_proof, cPoStProof(cpp_post_proof, post_type));
      c_proofs.push_back(c_post_proof);
    }
    return c_proofs;
  }

  // ******************
  // VERIFIED FUNCTIONS
  // ******************

  outcome::result<bool> Proofs::verifyWinningPoSt(
      const WinningPoStVerifyInfo &info) {
    OUTCOME_TRY(
        c_public_replica_infos,
        cPublicReplicaInfos(info.challenged_sectors, PoStType::Winning));
    OUTCOME_TRY(c_post_proofs,
                cPoStProofs(gsl::make_span(info.proofs), PoStType::Winning));
    auto prover_id = toProverID(info.prover);

    auto res_ptr =
        ffi::wrap(fil_verify_winning_post(c32ByteArray(info.randomness),
                                          c_public_replica_infos.data(),
                                          c_public_replica_infos.size(),
                                          c_post_proofs.data(),
                                          c_post_proofs.size(),
                                          prover_id),
                  fil_destroy_verify_winning_post_response);

    if (res_ptr->status_code != 0) {
      logger_->error("verifyWindowPoSt: " + std::string(res_ptr->error_msg));
      return ProofsError::kUnknown;
    }

    return res_ptr->is_valid;
  }

  outcome::result<bool> Proofs::verifyWindowPoSt(
      const WindowPoStVerifyInfo &info) {
    OUTCOME_TRY(c_public_replica_infos,
                cPublicReplicaInfos(info.challenged_sectors, PoStType::Window));
    OUTCOME_TRY(c_post_proofs,
                cPoStProofs(gsl::make_span(info.proofs), PoStType::Window));
    auto prover_id = toProverID(info.prover);

    auto res_ptr =
        ffi::wrap(fil_verify_window_post(c32ByteArray(info.randomness),
                                         c_public_replica_infos.data(),
                                         c_public_replica_infos.size(),
                                         c_post_proofs.data(),
                                         c_post_proofs.size(),
                                         prover_id),
                  fil_destroy_verify_window_post_response);

    if (res_ptr->status_code != 0) {
      logger_->error("verifyWindowPoSt: " + std::string(res_ptr->error_msg));
      return ProofsError::kUnknown;
    }

    return res_ptr->is_valid;
  }

  outcome::result<bool> Proofs::verifySeal(const SealVerifyInfo &info) {
    OUTCOME_TRY(c_proof_type, cRegisteredSealProof(info.info.registered_proof));

    OUTCOME_TRY(comm_r, CIDToReplicaCommitmentV1(info.info.sealed_cid));

    OUTCOME_TRY(comm_d, CIDToDataCommitmentV1(info.unsealed_cid));

    auto prover_id = toProverID(info.sector.miner);

    auto res_ptr =
        ffi::wrap(fil_verify_seal(c_proof_type,
                                  c32ByteArray(comm_r),
                                  c32ByteArray(comm_d),
                                  prover_id,
                                  c32ByteArray(info.randomness),
                                  c32ByteArray(info.interactive_randomness),
                                  info.info.sector,
                                  info.info.proof.data(),
                                  info.info.proof.size()),
                  fil_destroy_verify_seal_response);

    if (res_ptr->status_code != 0) {
      logger_->error("verifySeal: " + std::string(res_ptr->error_msg));

      return ProofsError::kUnknown;
    }

    return res_ptr->is_valid;
  }

  // ******************
  // GENERATED FUNCTIONS
  // ******************

  outcome::result<std::vector<PoStProof>> Proofs::generateWinningPoSt(
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

    if (res_ptr->status_code != 0) {
      logger_->error("generateWinningPoSt: " + std::string(res_ptr->error_msg));
      return ProofsError::kUnknown;
    }

    return cppPoStProofs(
        gsl::make_span(res_ptr->proofs_ptr, res_ptr->proofs_len));
  }

  outcome::result<std::vector<PoStProof>> Proofs::generateWindowPoSt(
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

    if (res_ptr->status_code != 0) {
      logger_->error("generateWindowPoSt: " + std::string(res_ptr->error_msg));
      return ProofsError::kUnknown;
    }

    return cppPoStProofs(
        gsl::make_span(res_ptr->proofs_ptr, res_ptr->proofs_len));
  }

  outcome::result<ChallengeIndexes> Proofs::generateWinningPoStSectorChallenge(
      RegisteredProof proof_type,
      ActorId miner_id,
      const PoStRandomness &randomness,
      uint64_t eligible_sectors_len) {
    auto rand31{randomness};
    rand31[31] = 0;

    OUTCOME_TRY(c_proof_type,
                cRegisteredPoStProof(proof_type, PoStType::Winning));
    auto prover_id = toProverID(miner_id);

    auto res_ptr = ffi::wrap(
        fil_generate_winning_post_sector_challenge(c_proof_type,
                                                   c32ByteArray(rand31),
                                                   eligible_sectors_len,
                                                   prover_id),
        fil_destroy_generate_winning_post_sector_challenge);

    if (res_ptr->status_code != 0) {
      logger_->error("generateWinningPoStSectorChallenge: "
                     + std::string(res_ptr->error_msg));

      return ProofsError::kUnknown;
    }

    return ChallengeIndexes(res_ptr->ids_ptr,
                            res_ptr->ids_ptr + res_ptr->ids_len);  // NOLINT
  }

  outcome::result<WriteWithoutAlignmentResult> Proofs::writeWithoutAlignment(
      RegisteredProof proof_type,
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
    if (res_ptr->status_code != 0) {
      logger_->error("writeWithoutAlignment: "
                     + std::string(res_ptr->error_msg));

      return ProofsError::kUnknown;
    }

    return cppWriteWithoutAlignmentResult(*res_ptr);
  }

  outcome::result<WriteWithAlignmentResult> Proofs::writeWithAlignment(
      RegisteredProof proof_type,
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

    OUTCOME_TRY(max_size, primitives::sector::getSectorSize(proof_type));

    UnpaddedPieceSize offset;

    for (const auto &piece_size : existing_piece_sizes) {
      offset += piece_size;
    }

    if ((offset.padded() + piece_bytes.padded()) > max_size) {
      // NOLINTNEXTLINE(readability-implicit-bool-conversion)
      if (close(staged_sector_fd))
        logger_->warn("writeWithAlignment: error in closing file "
                      + staged_sector_file_path);
      return ProofsError::kOutOfBound;
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
    if (res_ptr->status_code != 0) {
      logger_->error("writeWithAlignment: " + std::string(res_ptr->error_msg));
      return ProofsError::kUnknown;
    }
    return cppWriteWithAlignmentResult(*res_ptr);
  }

  outcome::result<Phase1Output> Proofs::sealPreCommitPhase1(
      RegisteredProof proof_type,
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

    if (res_ptr->status_code != 0) {
      logger_->error("Seal precommit phase 1: "
                     + std::string(res_ptr->error_msg));
      return ProofsError::kUnknown;
    }

    return Phase1Output(
        res_ptr->seal_pre_commit_phase1_output_ptr,
        res_ptr->seal_pre_commit_phase1_output_ptr
            + res_ptr->seal_pre_commit_phase1_output_len);  // NOLINT
  }

  outcome::result<SealedAndUnsealedCID> Proofs::sealPreCommitPhase2(
      gsl::span<const uint8_t> phase1_output,
      const std::string &cache_dir_path,
      const std::string &sealed_sector_path) {
    auto res_ptr =
        ffi::wrap(fil_seal_pre_commit_phase2(phase1_output.data(),
                                             phase1_output.size(),
                                             cache_dir_path.c_str(),
                                             sealed_sector_path.c_str()),
                  fil_destroy_seal_pre_commit_phase2_response);

    if (res_ptr->status_code != 0) {
      logger_->error("Seal precommit phase 2: "
                     + std::string(res_ptr->error_msg));
      return ProofsError::kUnknown;
    }

    return SealedAndUnsealedCID{
        .sealed_cid = replicaCommitmentV1ToCID(
            gsl::make_span(res_ptr->comm_r, kCommitmentBytesLen)),
        .unsealed_cid = dataCommitmentV1ToCID(
            gsl::make_span(res_ptr->comm_d, kCommitmentBytesLen)),
    };
  }

  outcome::result<Phase1Output> Proofs::sealCommitPhase1(
      RegisteredProof proof_type,
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

    if (res_ptr->status_code != 0) {
      logger_->error("sealCommit Phase 1: " + std::string(res_ptr->error_msg));
      return ProofsError::kUnknown;
    }

    return Phase1Output(
        res_ptr->seal_commit_phase1_output_ptr,
        res_ptr->seal_commit_phase1_output_ptr
            + res_ptr->seal_commit_phase1_output_len);  // NOLINT
  }

  outcome::result<Proof> Proofs::sealCommitPhase2(
      gsl::span<const uint8_t> phase1_output,
      SectorNumber sector_id,
      ActorId miner_id) {
    auto prover_id = toProverID(miner_id);
    auto res_ptr = ffi::wrap(
        fil_seal_commit_phase2(
            phase1_output.data(), phase1_output.size(), sector_id, prover_id),
        fil_destroy_seal_commit_phase2_response);

    if (res_ptr->status_code != 0) {
      logger_->error("sealCommit Phase 2: " + std::string(res_ptr->error_msg));
      return ProofsError::kUnknown;
    }

    return Proof(res_ptr->proof_ptr, res_ptr->proof_ptr + res_ptr->proof_len);
  }

  outcome::result<void> Proofs::unseal(RegisteredProof proof_type,
                                       const std::string &cache_dir_path,
                                       const std::string &sealed_sector_path,
                                       const std::string &unseal_output_path,
                                       SectorNumber sector_num,
                                       ActorId miner_id,
                                       const Ticket &ticket,
                                       const UnsealedCID &unsealed_cid) {
    OUTCOME_TRY(c_proof_type, cRegisteredSealProof(proof_type));

    OUTCOME_TRY(comm_d, CIDToDataCommitmentV1(unsealed_cid));
    auto prover_id = toProverID(miner_id);

    auto res_ptr = ffi::wrap(fil_unseal(c_proof_type,
                                        cache_dir_path.c_str(),
                                        sealed_sector_path.c_str(),
                                        unseal_output_path.c_str(),
                                        sector_num,
                                        prover_id,
                                        c32ByteArray(ticket),
                                        c32ByteArray(comm_d)),
                             fil_destroy_unseal_response);

    if (res_ptr->status_code != 0) {
      logger_->error("unseal: " + std::string(res_ptr->error_msg));

      return ProofsError::kUnknown;
    }

    return outcome::success();
  }

  outcome::result<void> Proofs::unsealRange(
      RegisteredProof proof_type,
      const std::string &cache_dir_path,
      const std::string &sealed_sector_path,
      const std::string &unseal_output_path,
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
                                              sealed_sector_path.c_str(),
                                              unseal_output_path.c_str(),
                                              sector_num,
                                              prover_id,
                                              c32ByteArray(ticket),
                                              c32ByteArray(comm_d),
                                              offset,
                                              length),
                             fil_destroy_unseal_range_response);

    if (res_ptr->status_code != 0) {
      logger_->error("unsealRange: " + std::string(res_ptr->error_msg));

      return ProofsError::kUnknown;
    }

    return outcome::success();
  }

  SortedPrivateSectorInfo Proofs::newSortedPrivateSectorInfo(
      gsl::span<const PrivateSectorInfo> replica_info) {
    SortedPrivateSectorInfo sorted_replica_info;
    sorted_replica_info.values.assign(replica_info.cbegin(),
                                      replica_info.cend());
    std::sort(sorted_replica_info.values.begin(),
              sorted_replica_info.values.end(),
              [](const PrivateSectorInfo &lhs, const PrivateSectorInfo &rhs) {
                return lhs.info.sealed_cid < rhs.info.sealed_cid;
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
    return generatePieceCID(proof_type, PieceData(piece_file_path), piece_size);
  }

  outcome::result<CID> Proofs::generatePieceCID(RegisteredProof proof_type,
                                                const PieceData &piece,
                                                UnpaddedPieceSize piece_size) {
    OUTCOME_TRY(c_proof_type, cRegisteredSealProof(proof_type));

    if (!piece.isOpened()) {
      return ProofsError::kCannotOpenFile;
    }

    auto res_ptr = ffi::wrap(
        fil_generate_piece_commitment(c_proof_type, piece.getFd(), piece_size),
        fil_destroy_generate_piece_commitment_response);

    if (res_ptr->status_code != 0) {
      logger_->error("GeneratePieceCIDFromFile: "
                     + std::string(res_ptr->error_msg));
      return ProofsError::kUnknown;
    }

    return dataCommitmentV1ToCID(
        gsl::make_span(res_ptr->comm_p, kCommitmentBytesLen));
  }

  outcome::result<CID> Proofs::generateUnsealedCID(
      RegisteredProof proof_type, gsl::span<const PieceInfo> pieces) {
    OUTCOME_TRY(c_proof_type, cRegisteredSealProof(proof_type));

    OUTCOME_TRY(c_pieces, cPublicPieceInfos(pieces));

    auto res_ptr =
        ffi::wrap(fil_generate_data_commitment(
                      c_proof_type, c_pieces.data(), c_pieces.size()),
                  fil_destroy_generate_data_commitment_response);

    if (res_ptr->status_code != 0) {
      logger_->error("generateUnsealedCID: " + std::string(res_ptr->error_msg));
      return ProofsError::kUnknown;
    }

    return dataCommitmentV1ToCID(
        gsl::make_span(res_ptr->comm_d, kCommitmentBytesLen));
  }

  outcome::result<void> Proofs::clearCache(SectorSize sector_size,
                                           const std::string &cache_dir_path) {
    auto res_ptr =
        ffi::wrap(fil_clear_cache(sector_size, cache_dir_path.c_str()),
                  fil_destroy_clear_cache_response);

    if (res_ptr->status_code != 0) {
      logger_->error("clearCache: " + std::string(res_ptr->error_msg));
      return ProofsError::kUnknown;
    }

    return outcome::success();
  }

  outcome::result<std::string> Proofs::getPoStVersion(
      RegisteredProof proof_type) {
    OUTCOME_TRY(c_proof_type,
                cRegisteredPoStProof(proof_type, PoStType::Either));
    auto res_ptr = ffi::wrap(fil_get_post_version(c_proof_type),
                             fil_destroy_string_response);

    if (res_ptr->status_code != 0) {
      logger_->error("getPoStVersion: " + std::string(res_ptr->error_msg));
      return ProofsError::kUnknown;
    }

    return std::string(res_ptr->string_val);
  }

  outcome::result<std::string> Proofs::getSealVersion(
      RegisteredProof proof_type) {
    OUTCOME_TRY(c_proof_type, cRegisteredSealProof(proof_type));
    auto res_ptr = ffi::wrap(fil_get_seal_version(c_proof_type),
                             fil_destroy_string_response);

    if (res_ptr->status_code != 0) {
      logger_->error("getSealVersion: " + std::string(res_ptr->error_msg));
      return ProofsError::kUnknown;
    }

    return std::string(res_ptr->string_val);
  }

  outcome::result<Devices> Proofs::getGPUDevices() {
    auto res_ptr =
        ffi::wrap(fil_get_gpu_devices(), fil_destroy_gpu_device_response);

    if (res_ptr->status_code != 0) {
      logger_->error("getGPUDevices: " + std::string(res_ptr->error_msg));
      return ProofsError::kUnknown;
    }

    if (res_ptr->devices_ptr == nullptr || res_ptr->devices_len == 0) {
      return Devices();
    }

    return Devices(res_ptr->devices_ptr,
                   res_ptr->devices_ptr + res_ptr->devices_len);  // NOLINT
  }

  outcome::result<void> Proofs::readPiece(PieceData output,
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
    std::vector<uint8_t> buffer(kDefaultBufferSize);
    auto chunks = kDefaultBufferSize / 127;
    PaddedPieceSize outTwoPow =
        primitives::piece::paddedSize(chunks * 128).padded();

    while (left > 0) {
      if (left < outTwoPow.unpadded()) {
        outTwoPow = primitives::piece::paddedSize(left).padded();
      }
      std::vector<uint8_t> read(outTwoPow);

      size_t j;
      char ch;
      for (j = 0; j < outTwoPow && input; j++) {
        input.get(ch);
        read[j] = ch;
      }

      if (j != outTwoPow) {
        return ProofsError::kNotReadEnough;
      }

      unpad(gsl::make_span(read.data(), outTwoPow),
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

  outcome::result<void> Proofs::writeUnsealPiece(
      const std::string &unseal_piece_file_path,
      const std::string &staged_sector_file_path,
      RegisteredProof seal_proof_type,
      const PaddedPieceSize &offset,
      const UnpaddedPieceSize &piece_size) {
    std::ifstream input(unseal_piece_file_path);

    if (!input.good()) {
      return ProofsError::kCannotOpenFile;
    }

    if (!fs::exists(staged_sector_file_path)) {
      std::ofstream ofs(staged_sector_file_path,
                        std::ios::binary | std::ios::out);

      if (!ofs.good()) {
        return ProofsError::kCannotCreateUnsealedFile;
      }

      OUTCOME_TRY(size, getSectorSize(seal_proof_type));

      char ch = 0;
      size_t i;
      for (i = 0; i < size && ofs; i++) {
        ofs.write(&ch, 1);
      }

      if (i != size) {
        return ProofsError::kNotWriteEnough;
      }
    }

    auto size = fs::file_size(staged_sector_file_path);

    if (offset + piece_size.padded() > size) {
      return ProofsError::kOutOfBound;
    }

    std::fstream unsealed_file(staged_sector_file_path,
                               std::ios::in | std::ios::out);

    if (!unsealed_file.good()) {
      return ProofsError::kCannotOpenFile;
    }

    if (!unsealed_file.seekg(offset, std::ios_base::beg)) {
      return ProofsError::kUnableMoveCursor;
    }

    std::vector<uint8_t> in(127);
    std::vector<uint8_t> out(128);

    for (size_t read = 0; read < piece_size; read += 127) {
      char ch;
      size_t i;
      for (i = 0; i < 127 && input; i++) {
        input.get(ch);
        in[i] = ch;
      }

      if (i != 127) {
        return ProofsError::kNotReadEnough;
      }

      pad(in, out);
      for (i = 0; i < 128 && unsealed_file; i++) {
        ch = out[i];
        unsealed_file.write(&ch, 1);
      }
      if (i != 128) {
        return ProofsError::kNotWriteEnough;
      }
    }

    return outcome::success();
  }

}  // namespace fc::proofs
