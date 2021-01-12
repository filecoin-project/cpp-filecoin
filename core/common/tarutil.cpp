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

  static Logger logger = createLogger("tar util");

  int copy_data(struct archive *ar, struct archive *aw) {
    int return_code;
    const void *buff = nullptr;
    size_t size;
    la_int64_t offset;

    for (;;) {
      return_code = archive_read_data_block(ar, &buff, &size, &offset);
      if (return_code == ARCHIVE_EOF) {
        return (ARCHIVE_OK);
      }
      if (return_code < ARCHIVE_OK) {
        return (return_code);
      }
      return_code = archive_write_data_block(aw, buff, size, offset);
      if (return_code < ARCHIVE_OK) {
        return (return_code);
      }
    }
  }

  outcome::result<void> zipTar(const std::string &input_path,
                               const std::string &output_path) {
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
      } else {
        logger->warn("Zip tar: {}", archive_error_string(archive.get()));
      }
    }
    std::function<outcome::result<void>(const fs::path &, const fs::path &)>
        zipDir = [&](const fs::path &absolute_path,
                     const fs::path &relative_path) -> outcome::result<void> {
      struct archive_entry *entry = nullptr;
      struct stat entry_stat {};
      char buff[8192];
      fs::directory_iterator dir_iter(absolute_path), end;

      while (dir_iter != end) {
        auto type = AE_IFREG;
        if (fs::is_directory(dir_iter->path())) {
          if (fs::directory_iterator(dir_iter->path())
              != fs::directory_iterator()) {
            OUTCOME_TRY(zipDir(dir_iter->path(),
                               relative_path / dir_iter->path().filename()));
            ++dir_iter;
            continue;
          }
          type = AE_IFDIR;
        }
        if (stat(dir_iter->path().c_str(), &entry_stat) < 0) {
          return TarErrors::kCannotZipTarArchive;
        }
        entry = archive_entry_new();
        archive_entry_set_pathname(
            entry, (relative_path / dir_iter->path().filename()).c_str());
        archive_entry_set_size(entry, entry_stat.st_size);
        archive_entry_set_filetype(entry, type);
        archive_entry_set_perm(entry, 0644);
        return_code = archive_write_header(archive.get(), entry);
        if (return_code < ARCHIVE_OK) {
          if (return_code < ARCHIVE_WARN) {
            logger->error("Zip tar: {}", archive_error_string(archive.get()));
            return TarErrors::kCannotZipTarArchive;
          } else {
            logger->warn("Zip tar: {}", archive_error_string(archive.get()));
          }
        }
        if (type == AE_IFREG) {
          int fd = open(dir_iter->path().c_str(), O_RDONLY);
          int len = read(fd, buff, sizeof(buff));
          while (len > 0) {
            if (archive_write_data(archive.get(), buff, len) == -1) {
              logger->error("Zip tar: {}", archive_error_string(archive.get()));
              return TarErrors::kCannotZipTarArchive;
            }
            len = read(fd, buff, sizeof(buff));
          }
          close(fd);
        }
        archive_entry_free(entry);

        ++dir_iter;
      }
      return outcome::success();
    };

    fs::path base = fs::path(input_path).filename();
    OUTCOME_TRY(zipDir(input_path, base));
    archive_write_close(archive.get());

    return outcome::success();
  }

  outcome::result<void> extractTar(const std::string &tar_path,
                                   const std::string &output_path) {
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
    int flags;
    int return_code;

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
    default:
      return "Tar Util: unknown error";
  }
}
