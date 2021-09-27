/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/tarutil.hpp"

#include <fcntl.h>
#include <libarchive/archive.h>
#include <libarchive/archive_entry.h>
#include <boost/filesystem.hpp>
#include "common/ffi.hpp"
#include "common/logger.hpp"

namespace fs = boost::filesystem;

namespace fc::common {

  const static Logger logger = createLogger("tar util");

  int copy_data(struct archive *ar, struct archive *aw) {
    la_ssize_t return_code = 0;
    const void *buff = nullptr;
    size_t size{};
    la_int64_t offset{};

    for (;;) {
      return_code = archive_read_data_block(ar, &buff, &size, &offset);
      if (return_code == ARCHIVE_EOF) {
        return (ARCHIVE_OK);
      }
      if (return_code < ARCHIVE_OK) {
        return static_cast<int>(return_code);
      }
      return_code = archive_write_data_block(aw, buff, size, offset);
      if (return_code < ARCHIVE_OK) {
        return static_cast<int>(return_code);
      }
    }
  }

  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  outcome::result<void> zipTar(const boost::filesystem::path &input_path,
                               const boost::filesystem::path &output_path) {
    if (!fs::exists(input_path)) {
      logger->error("Zip tar: {} doesn't exists", input_path);
      return TarErrors::kCannotZipTarArchive;
    }
    if (!fs::is_directory(input_path)) {
      logger->error("Zip tar: {} is not a directory", input_path);
      return TarErrors::kCannotZipTarArchive;
    }
    if (fs::exists(output_path) && !fs::is_regular_file(output_path)) {
      logger->error("Zip tar: {} is not a file", output_path);
      return TarErrors::kCannotZipTarArchive;
    }

    auto archive = ffi::wrap(archive_write_new(), archive_write_free);
    archive_write_set_format_v7tar(archive.get());
    int return_code =
        archive_write_open_filename(archive.get(), output_path.c_str());
    if (return_code < ARCHIVE_OK) {
      if (return_code < ARCHIVE_WARN) {
        logger->error("Zip tar: {}", archive_error_string(archive.get()));
        return TarErrors::kCannotZipTarArchive;
      }
      logger->warn("Zip tar: {}", archive_error_string(archive.get()));
    }
    std::function<outcome::result<void>(const fs::path &, const fs::path &)>
        // NOLINTNEXTLINE(readability-function-cognitive-complexity)
        zipDir = [&](const fs::path &absolute_path,
                     const fs::path &relative_path) -> outcome::result<void> {
      struct stat entry_stat {};
      std::array<char, 8192> buff{};

      for (const auto &dir_item : fs::directory_iterator(absolute_path)) {
        auto type = AE_IFREG;
        if (fs::is_directory(dir_item.path())) {
          if (fs::directory_iterator(dir_item.path())
              != fs::directory_iterator()) {
            OUTCOME_TRY(zipDir(dir_item.path(),
                               relative_path / dir_item.path().filename()));
            continue;
          }
          type = AE_IFDIR;
        }
        if (stat(dir_item.path().c_str(), &entry_stat) < 0) {
          return TarErrors::kCannotZipTarArchive;
        }
        auto entry = ffi::wrap(archive_entry_new(), archive_entry_free);
        archive_entry_set_pathname(
            entry.get(), (relative_path / dir_item.path().filename()).c_str());
        archive_entry_set_size(entry.get(), entry_stat.st_size);
        archive_entry_set_filetype(entry.get(), type);
        archive_entry_set_perm(entry.get(), 0644);
        return_code = archive_write_header(archive.get(), entry.get());
        if (return_code < ARCHIVE_OK) {
          if (return_code < ARCHIVE_WARN) {
            logger->error("Zip tar: {}", archive_error_string(archive.get()));
            return TarErrors::kCannotZipTarArchive;
          }
          logger->warn("Zip tar: {}", archive_error_string(archive.get()));
        }
        if (type == AE_IFREG) {
          std::ifstream file(dir_item.path().c_str());
          if (not file.is_open()) {
            return TarErrors::kCannotOpenFile;
          }

          while (not file.eof()) {
            file.read(buff.data(), sizeof(buff));

            if (not file.good() && not file.eof()) {
              return TarErrors::kCannotReadFile;
            }

            if (archive_write_data(archive.get(), buff.data(), file.gcount())
                == -1) {
              logger->error("Zip tar: {}", archive_error_string(archive.get()));
              return TarErrors::kCannotZipTarArchive;
            }
          }
        }
      }
      return outcome::success();
    };

    fs::path base = fs::path(input_path).filename();
    OUTCOME_TRY(zipDir(input_path, base));
    archive_write_close(archive.get());

    return outcome::success();
  }

  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  outcome::result<void> extractTar(const boost::filesystem::path &tar_path,
                                   const boost::filesystem::path &output_path) {
    if (!fs::exists(output_path)) {
      boost::system::error_code ec;
      if (!fs::create_directories(output_path, ec)) {
        if (ec.failed()) {
          logger->error("Extract tar: {}", ec.message());
        }
        return TarErrors::kCannotCreateDir;
      }
    }

    struct archive_entry *entry = nullptr;
    int flags = 0;
    int return_code = 0;

    /* Select which attributes we want to restore. */
    flags = ARCHIVE_EXTRACT_TIME;
    flags |= ARCHIVE_EXTRACT_PERM;
    flags |= ARCHIVE_EXTRACT_ACL;
    flags |= ARCHIVE_EXTRACT_FFLAGS;

    auto archive = ffi::wrap(archive_read_new(), archive_read_free);
    archive_read_support_format_tar(archive.get());
    auto ext = ffi::wrap(archive_write_disk_new(), archive_write_free);
    archive_write_disk_set_options(ext.get(), flags);
    archive_write_disk_set_standard_lookup(ext.get());
    if (archive_read_open_filename(
            archive.get(), tar_path.c_str(), kTarBlockSize)
        != ARCHIVE_OK) {
      logger->error("Extract tar: {}", archive_error_string(archive.get()));
      return TarErrors::kCannotUntarArchive;
    }
    for (;;) {
      return_code = archive_read_next_header(archive.get(), &entry);
      if (return_code == ARCHIVE_EOF) {
        break;
      }
      if (return_code < ARCHIVE_WARN) {
        logger->error("Extract tar: {}", archive_error_string(archive.get()));
        return TarErrors::kCannotUntarArchive;
      }
      if (return_code < ARCHIVE_OK) {
        logger->warn("Extract tar: {}", archive_error_string(archive.get()));
      }

      std::string currentFile(archive_entry_pathname(entry));
      archive_entry_set_pathname(entry,
                                 (fs::path(output_path) / currentFile).c_str());
      return_code = archive_write_header(ext.get(), entry);
      if (return_code < ARCHIVE_OK) {
        if (return_code < ARCHIVE_WARN) {
          logger->error("Extract tar: {}", archive_error_string(archive.get()));
        } else {
          logger->warn("Extract tar: {}", archive_error_string(archive.get()));
        }
      } else if (archive_entry_size(entry) > 0) {
        return_code = copy_data(archive.get(), ext.get());
        if (return_code < ARCHIVE_WARN) {
          logger->error("Extract tar: {}", archive_error_string(archive.get()));
          return TarErrors::kCannotUntarArchive;
        }
        if (return_code < ARCHIVE_OK) {
          logger->warn("Extract tar: {}", archive_error_string(archive.get()));
        }
      }
      return_code = archive_write_finish_entry(ext.get());
      if (return_code < ARCHIVE_WARN) {
        logger->error("Extract tar: {}", archive_error_string(archive.get()));
        return TarErrors::kCannotUntarArchive;
      }
      if (return_code < ARCHIVE_OK) {
        logger->warn("Extract tar: {}", archive_error_string(archive.get()));
      }
    }
    archive_read_close(archive.get());
    archive_write_close(ext.get());

    return outcome::success();
  }

}  // namespace fc::common

OUTCOME_CPP_DEFINE_CATEGORY(fc::common, TarErrors, e) {
  using fc::common::TarErrors;
  switch (e) {
    case (TarErrors::kCannotCreateDir):
      return "Tar Util: cannot create output dir";
    case (TarErrors::kCannotUntarArchive):
      return "Tar Util: cannot untar archive";
    case (TarErrors::kCannotZipTarArchive):
      return "Tar Util: cannot zip tar archive";
    case (TarErrors::kCannotOpenFile):
      return "Tar Util: cannot open file for write to archive";
    case (TarErrors::kCannotReadFile):
      return "Tar Util: cannot read data from file";
    default:
      return "Tar Util: unknown error";
  }
}
