/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/sector_file/sector_file.hpp"

#include <boost/endian/buffers.hpp>
#include <boost/filesystem.hpp>
#include <libp2p/multi/uvarint.hpp>

#include "common/bitsutil.hpp"
#include "primitives/bitvec/bitvec.hpp"
#include "primitives/cid/comm_cid.hpp"
#include "primitives/rle_bitset/runs_utils.hpp"
#include "primitives/sector/sector.hpp"
#include "proofs/impl/proof_engine_impl.hpp"

namespace fc::primitives::sector_file {

  namespace fs = boost::filesystem;
  using bitvec::BitvecWriter;
  using piece::PieceInfo;
  using sector::RegisteredSealProof;

  std::string toString(const SectorFileType &file_type) {
    switch (file_type) {
      case FTUnsealed:
        return "unsealed";
      case FTSealed:
        return "sealed";
      case FTCache:
        return "cache";
      default:
        return "<unknown " + std::to_string(file_type) + ">";
    }
  }

  outcome::result<SectorFileType> fromString(const std::string &file_type_str) {
    if (file_type_str == "unsealed") {
      return SectorFileType::FTUnsealed;
    }
    if (file_type_str == "sealed") {
      return SectorFileType::FTSealed;
    }
    if (file_type_str == "cache") {
      return SectorFileType::FTCache;
    }
    return SectorFileTypeErrors::kInvalidSectorFileType;
  }

  outcome::result<uint64_t> sealSpaceUse(SectorFileType file_type,
                                         SectorSize sector_size) {
    uint64_t result = 0;
    for (const auto &type : kSectorFileTypes) {
      if ((file_type & type) == 0) {
        continue;
      }

      auto overhead_iter = kOverheadSeal.find(type);
      if (overhead_iter == kOverheadSeal.end()) {
        return SectorFileTypeErrors::kInvalidSectorFileType;
      }

      result += overhead_iter->second * sector_size / kOverheadDenominator;
    }
    return result;
  }

  void SectorPaths::setPathByType(const SectorFileType &file_type,
                                  const std::string &path) {
    switch (file_type) {
      case FTCache:
        cache = path;
        return;
      case FTUnsealed:
        unsealed = path;
        return;
      case FTSealed:
        sealed = path;
        return;
      default:
        return;
    }
  }

  outcome::result<std::string> SectorPaths::getPathByType(
      const SectorFileType &file_type) const {
    switch (file_type) {
      case FTCache:
        return cache;
      case FTUnsealed:
        return unsealed;
      case FTSealed:
        return sealed;
      default:
        return SectorFileTypeErrors::kInvalidSectorFileType;
    }
  }

  outcome::result<SectorId> parseSectorName(const std::string &sector_str) {
    SectorNumber sector_id;
    ActorId miner_id;

    auto count =
        std::sscanf(sector_str.c_str(), "s-t0%lld-%lld", &miner_id, &sector_id);

    if (count != 2) {
      return SectorFileTypeErrors::kInvalidSectorName;
    }

    return SectorId{
        .miner = miner_id,
        .sector = sector_id,
    };
  }

  std::string sectorName(const SectorId &sid) {
    return "s-t0" + std::to_string(sid.miner) + "-"
           + std::to_string(sid.sector);
  }

  outcome::result<std::vector<uint8_t>> toTrailer(
      gsl::span<const uint64_t> runs) {
    BitvecWriter trailer;
    trailer.put(0, 2);

    bool is_first = true;
    bool is_previous_value = false;

    for (int i = 0; i < runs.size(); i++) {
      if (is_first) {
        if (runs[i] == 0) {
          i++;
          trailer.put(1, 1);
        } else {
          trailer.put(0, 1);
        }
        is_first = false;
      } else if (not(i % 2) == is_previous_value) {
        return SectorFileError::kInvalidRuns;
      }

      is_previous_value = not(i % 2);

      if (runs[i] == 1) {
        trailer.put(1, 1);
      } else if (runs[i] < 16) {
        trailer.put(2, 2);
        trailer.put(uint8_t(runs[i]), 4);
      } else if (runs[i] >= 16) {
        trailer.put(0, 2);
        libp2p::multi::UVarint uvarint{runs[i]};
        auto encoded = uvarint.toBytes();
        for (auto it = encoded.cbegin(); it != encoded.cend(); it++) {
          trailer.put(*it, 8);
        }
      }
    }

    if (is_first) {
      trailer.put(0, 1);
    }

    return trailer.out();
  }

  outcome::result<void> writeTrailer(const std::string &path,
                                     int64_t max_piece_size,
                                     gsl::span<const uint64_t> runs) {
    OUTCOME_TRY(trailer, toTrailer(runs));

    std::fstream file(path);

    if (!file.good()) {
      return SectorFileError::kCannotOpenFile;
    }

    file.seekp(max_piece_size, std::ios_base::beg);

    if (!file.good()) {
      return SectorFileError::kCannotMoveCursor;
    }

    file.write((char *)trailer.data(), trailer.size());

    if (!file.good()) {
      return SectorFileError::kCannotWrite;
    }

    boost::endian::little_uint32_buf_t trailer_size(trailer.size());

    file.write((char *)trailer_size.data(), sizeof(uint32_t));

    if (!file.good()) {
      return SectorFileError::kCannotWrite;
    }

    file.close();

    boost::system::error_code ec;
    fs::resize_file(
        path, max_piece_size + sizeof(uint32_t) + trailer.size(), ec);
    if (ec.failed()) {
      return SectorFileError::kCannotResizeFile;
    }

    return outcome::success();
  }

  outcome::result<std::shared_ptr<SectorFile>> SectorFile::createFile(
      const std::string &path, PaddedPieceSize max_piece_size) {
    {
      int fd = open(path.c_str(), O_RDWR | O_CREAT, 0644);
      if (fd == -1) {
        return SectorFileError::kCannotCreateFile;
      }
      close(fd);
    }
    boost::system::error_code ec;
    fs::resize_file(path, max_piece_size, ec);
    if (ec.failed()) {
      return SectorFileError::kCannotResizeFile;
    }

    OUTCOME_TRY(writeTrailer(path, max_piece_size, {}));

    return openFile(path, max_piece_size);
  }

  outcome::result<std::shared_ptr<SectorFile>> SectorFile::openFile(
      const std::string &path, PaddedPieceSize max_piece_size) {
    if (!fs::exists(path)) {
      return SectorFileError::kFileNotExist;
    }

    int fd = open(path.c_str(), O_RDWR, 0644);
    if (fd == -1) {
      return SectorFileError::kCannotOpenFile;
    }

    auto _ = gsl::finally([fd]() { close(fd); });

    uint64_t file_size = fs::file_size(path);
    constexpr uint64_t trailer_size_bytes =
        sizeof(uint32_t);  // 32 bit - 4 byte

    boost::endian::little_uint32_buf_t trailer_size_buf;

    pread(fd,
          trailer_size_buf.data(),
          trailer_size_bytes,
          file_size - trailer_size_bytes);

    uint64_t trailer_size = trailer_size_buf.value();

    if ((trailer_size + trailer_size_bytes + max_piece_size) != file_size) {
      return SectorFileError::kInvalidSize;
    }

    uint64_t trailer_offset = file_size - trailer_size_bytes - trailer_size;

    std::vector<uint8_t> trailer(trailer_size);
    pread(fd, trailer.data(), trailer_size, trailer_offset);

    OUTCOME_TRY(runs, runsFromBuffer(trailer));

    auto fill_runs = runsFill(runs);

    OUTCOME_TRY(size, runsCount(fill_runs));

    if (size > max_piece_size) {
      return SectorFileError::kOversizeTrailer;
    }

    struct make_unique_enabler : public SectorFile {
      make_unique_enabler(const std::string &path,
                          PaddedPieceSize max_size,
                          const std::vector<uint64_t> &runs)
          : SectorFile{path, max_size, runs} {};
    };

    auto file =
        std::make_unique<make_unique_enabler>(path, max_piece_size, runs);

    if (file->file_.is_open()) {
      return std::move(file);
    }

    return SectorFileError::kCannotOpenFile;
  }

  outcome::result<void> SectorFile::markAllocated(PaddedByteIndex offset,
                                                  PaddedPieceSize size) {
    auto new_runs =
        primitives::runsOr(runs_, std::vector<uint64_t>{offset, size});

    OUTCOME_TRY(writeTrailer(path_, max_size_, new_runs));

    runs_ = std::move(new_runs);

    return outcome::success();
  }

  outcome::result<void> SectorFile::free(PaddedByteIndex offset,
                                         PaddedPieceSize size) {
    // TODO: deallocate

    auto new_runs =
        primitives::runsAnd(runs_, std::vector<uint64_t>{offset, size}, true);

    OUTCOME_TRY(writeTrailer(path_, max_size_, new_runs));

    runs_ = std::move(new_runs);

    return outcome::success();
  }

  gsl::span<const uint64_t> SectorFile::allocated() const {
    return gsl::make_span(runs_);
  }

  outcome::result<bool> SectorFile::hasAllocated(UnpaddedByteIndex offset,
                                                 UnpaddedPieceSize size) const {
    auto runs = primitives::runsAnd(
        runs_,
        std::vector<uint64_t>{primitives::piece::paddedIndex(offset),
                              size.padded()});

    OUTCOME_TRY(allocated_size, primitives::runsCount(runs));

    return allocated_size == size.padded();
  }

  SectorFile::SectorFile(std::string path,
                         PaddedPieceSize max_size,
                         std::vector<uint64_t> runs)
      : runs_(std::move(runs)), max_size_(max_size), path_(std::move(path)) {
    file_.open(path_);
    logger_ = common::createLogger("sector file");
  }

  outcome::result<boost::optional<PieceInfo>> SectorFile::write(
      const PieceData &data,
      PaddedByteIndex offset,
      PaddedPieceSize size,
      const boost::optional<RegisteredSealProof> &maybe_seal_proof_type) {
    if (not data.isOpened()) {
      return SectorFileError::kPipeNotOpen;
    }

    if (not file_.good()) {
      return SectorFileError::kInvalidFile;
    }

    {
      auto piece = runsAnd(runs_, std::vector<uint64_t>({offset, size}));

      OUTCOME_TRY(allocated_size, runsCount(piece));

      if (allocated_size > 0) {
        logger_->warn(
            "getting partial file writer overwriting {} allocated bytes",
            allocated_size);
      }
    }

    file_.seekp(offset, std::ios_base::beg);

    if (not file_.good()) {
      return SectorFileError::kCannotMoveCursor;
    }

    PaddedPieceSize chunk_size(4 << 20);

    if (size < chunk_size) {
      chunk_size = size;
    }

    static std::shared_ptr<proofs::ProofEngine> proofs =
        std::make_shared<proofs::ProofEngineImpl>();

    std::vector<PieceInfo> cids;

    std::vector<uint8_t> buffer(chunk_size.unpadded());

    SectorFile::PadWriter writer(file_);

    uint64_t written = 0;
    while (written < size) {
      if ((size - written) < chunk_size) {
        chunk_size = size - written;
      }

      uint64_t read = 0;
      while (read < chunk_size.unpadded()) {
        auto current_read = ::read(data.getFd(),
                                   (char *)(buffer.data() + read),
                                   chunk_size.unpadded() - read);
        if (current_read == -1) {
          // TODO: check errno
          return SectorFileError::kCannotRead;
        }

        if (current_read == 0) {
          break;
        }
        read += current_read;
      }

      OUTCOME_TRY(writer.write(gsl::make_span<uint8_t>(buffer.data(), read)));

      assert(UnpaddedPieceSize(read).validate());
      auto piece_size = UnpaddedPieceSize(read).padded();
      written += piece_size;

      if (maybe_seal_proof_type.has_value()) {
        OUTCOME_TRY(cid,
                    proofs->generatePieceCID(
                        maybe_seal_proof_type.value(),
                        gsl::make_span<uint8_t>(buffer.data(), read)));

        cids.push_back(PieceInfo{
            .size = piece_size,
            .cid = cid,
        });
      }
    }

    OUTCOME_TRY(markAllocated(offset, size));

    if (not maybe_seal_proof_type.has_value()) {
      return boost::none;
    }

    if (cids.size() == 1) {
      return cids[0];
    }

    OUTCOME_TRY(
        cid, proofs->generateUnsealedCID(maybe_seal_proof_type.value(), cids));

    OUTCOME_TRY(cid::CIDToPieceCommitmentV1(cid));

    return PieceInfo{
        .size = size,
        .cid = cid,
    };
  }

  outcome::result<bool> SectorFile::read(const PieceData &output,
                                         PaddedByteIndex offset,
                                         PaddedPieceSize size) {
    if (not output.isOpened()) {
      return SectorFileError::kPipeNotOpen;
    }

    if (not file_.good()) {
      return SectorFileError::kInvalidFile;
    }

    {
      auto piece = runsAnd(runs_, std::vector<uint64_t>({offset, size}));

      OUTCOME_TRY(allocated_size, runsCount(piece));

      if (allocated_size != size) {
        logger_->warn(
            "getting partial file reader reading {} unallocated bytes",
            size - allocated_size);
      }
    }

    file_.seekg(offset, std::ios_base::beg);

    if (!file_.good()) {
      return SectorFileError::kCannotMoveCursor;
    }

    uint64_t left = size.unpadded();
    constexpr auto kDefaultBufferSize = uint64_t(32 * 1024);
    PaddedPieceSize output_size =
        primitives::piece::paddedSize(kDefaultBufferSize).padded();
    std::vector<uint8_t> read(output_size);
    std::vector<uint8_t> buffer(output_size.unpadded());

    while (left > 0) {
      if (left < output_size.unpadded()) {
        output_size = primitives::piece::paddedSize(left).padded();
      }

      file_.read((char *)read.data(), output_size);

      if (!file_.good()) {
        return SectorFileError::kCannotRead;
      }

      if (uint64_t(file_.gcount()) != output_size) {
        return SectorFileError::kNotReadEnough;
      }

      primitives::piece::unpad(
          gsl::make_span(read.data(), output_size),
          gsl::make_span(
              buffer.data(),
              output_size.unpadded()));  // TODO: maybe boost map file

      auto write_size =
          ::write(output.getFd(), buffer.data(), output_size.unpadded());

      if (write_size == -1) {
        // TODO: check errno
        return SectorFileError::kCannotWrite;
      }

      if (uint64_t(write_size) != output_size.unpadded()) {
        return SectorFileError::kNotWriteEnough;
      }

      if (left < output_size.unpadded()) {
        break;
      }
      left -= output_size.unpadded();
    }

    return true;
  }

  std::vector<UnpaddedPieceSize> subPieces(UnpaddedPieceSize size) {
    uint64_t padded_size = size.padded();

    std::vector<UnpaddedPieceSize> result;

    while (padded_size != 0) {
      auto next = common::countTrailingZeros(padded_size);

      PaddedPieceSize piece_size(1 << next);

      padded_size ^= piece_size;

      result.push_back(piece_size.unpadded());
    }

    return result;
  }

  SectorFile::PadWriter::PadWriter(std::fstream &output) : output_(output) {}

  outcome::result<void> SectorFile::PadWriter::write(
      gsl::span<const uint8_t> bytes) {
    if (stash_.size() + bytes.size() < 127) {
      std::copy(bytes.begin(), bytes.end(), std::back_inserter(stash_));
    }

    std::vector<uint8_t> input;
    if (not stash_.empty()) {
      input = std::move(stash_);
    }

    std::copy(bytes.begin(), bytes.end(), std::back_inserter(input));
    while (true) {
      auto pieces = subPieces(UnpaddedPieceSize(input.size()));
      const UnpaddedPieceSize biggest = pieces.back();

      if (work_.size() < biggest.padded()) {
        work_.resize(biggest.padded());
      }

      primitives::piece::pad(
          gsl::make_span<const uint8_t>(input.data(), biggest),
          gsl::make_span<uint8_t>(work_.data(), biggest.padded()));

      output_.write((char *)work_.data(), biggest.padded());

      if (not output_.good()) {
        return SectorFileError::kCannotWrite;
      }

      input.erase(input.begin(), input.begin() + biggest);

      if (input.size() < 127) {
        stash_ = std::move(input);
        return outcome::success();
      }
    }
  }

}  // namespace fc::primitives::sector_file

OUTCOME_CPP_DEFINE_CATEGORY(fc::primitives::sector_file, SectorFileError, e) {
  using fc::primitives::sector_file::SectorFileError;
  switch (e) {
    case (SectorFileError::kFileNotExist):
      return "SectorFile: the file doesn't exist";
    case (SectorFileError::kCannotOpenFile):
      return "SectorFile: cannot open the sector file";
    case (SectorFileError::kInvalidFile):
      return "SectorFile: the file in an invalid state";
    case (SectorFileError::kPipeNotOpen):
      return "SectorFile: input/output piece is not open";
    case (SectorFileError::kNotReadEnough):
      return "SectorFile: not read enough data";
    case (SectorFileError::kNotWriteEnough):
      return "SectorFile: not write enough data";
    case (SectorFileError::kCannotWrite):
      return "SectorFile: cannot write data";
    case (SectorFileError::kCannotMoveCursor):
      return "SectorFile: cannot seek file";
    case (SectorFileError::kCannotRead):
      return "SectorFile: cannot read data";
    case (SectorFileError::kCannotCreateFile):
      return "SectorFile: cannot create a sector file";
    case (SectorFileError::kCannotResizeFile):
      return "SectorFile: cannot resize file";
    case (SectorFileError::kInvalidRuns):
      return "SectorFile: runs are invalid";
    case (SectorFileError::kInvalidSize):
      return "SectorFile: size of file is invalid";
    case (SectorFileError::kOversizeTrailer):
      return "SectorFile: the trailer is wrapped more than max size";
    default:
      return "SectorFile: unknown error";
  }
}

OUTCOME_CPP_DEFINE_CATEGORY(fc::primitives::sector_file,
                            SectorFileTypeErrors,
                            e) {
  using fc::primitives::sector_file::SectorFileTypeErrors;
  switch (e) {
    case (SectorFileTypeErrors::kInvalidSectorFileType):
      return "SectorFileType: unsupported sector file type";
    case (SectorFileTypeErrors::kInvalidSectorName):
      return "SectorFileType: cannot parse sector name";
    default:
      return "SectorFileType: unknown error";
  }
}
