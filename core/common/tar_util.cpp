/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/tar_util.hpp"

#include <boost/filesystem.hpp>
#include "common/logger.hpp"
// TODO: Add libarchive include

namespace fs = boost::filesystem;

namespace fc::common {

  static Logger logger = createLogger("tar util");

  int copy_data(struct archive *ar, struct archive *aw) {
    int r;
    const void *buff;
    size_t size;
    la_int64_t offset;

    for (;;) {
      r = archive_read_data_block(ar, &buff, &size, &offset);
      if (r == ARCHIVE_EOF) return (ARCHIVE_OK);
      if (r < ARCHIVE_OK) return (r);
      r = archive_write_data_block(aw, buff, size, offset);
      if (r < ARCHIVE_OK) {
        return (r);
      }
    }
  }

  outcome::result<void> extractTar(const std::string &tar_path,
                                   const std::string &output_path) {
    if (!fs::exists(output_path)) {
      boost::system::error_code ec;
      if (!fs::create_directories(output_path, ec)) {
        if (ec.failed()) {
          logger->error("Extract tar: {}", ec.message());
        }
        return TarErrors::CANNOT_CREATE_DIR;
      }
    }

    struct archive *a;
    struct archive *ext;
    struct archive_entry *entry;
    int flags;
    int r;

    /* Select which attributes we want to restore. */
    flags = ARCHIVE_EXTRACT_TIME;
    flags |= ARCHIVE_EXTRACT_PERM;
    flags |= ARCHIVE_EXTRACT_ACL;
    flags |= ARCHIVE_EXTRACT_FFLAGS;

    a = archive_read_new();
    archive_read_support_format_tar(a);
    ext = archive_write_disk_new();
    archive_write_disk_set_options(ext, flags);
    archive_write_disk_set_standard_lookup(ext);
    if (archive_read_open_filename(a, tar_path.c_str(), kTarBlockSize)
        != ARCHIVE_OK) {
      logger->error("Extract tar: {}", archive_error_string(a));
      return TarErrors::CANNOT_UNTAR_ARCHIVE;
    }
    for (;;) {
      r = archive_read_next_header(a, &entry);
      if (r == ARCHIVE_EOF) break;
      if (r < ARCHIVE_WARN) {
        logger->error("Extract tar: {}", archive_error_string(a));
        return TarErrors::CANNOT_UNTAR_ARCHIVE;
      }
      if (r < ARCHIVE_OK)
        logger->warn("Extract tar: {}", archive_error_string(a));

      std::string currentFile(archive_entry_pathname(entry));
      archive_entry_set_pathname(entry,
                                 (fs::path(output_path) / currentFile).c_str());
      r = archive_write_header(ext, entry);
      if (r < ARCHIVE_OK)
        if (r < ARCHIVE_WARN) {
          logger->error("Extract tar: {}", archive_error_string(a));
        } else {
          logger->warn("Extract tar: {}", archive_error_string(a));
        }
      else if (archive_entry_size(entry) > 0) {
        r = copy_data(a, ext);
        if (r < ARCHIVE_WARN) {
          logger->error("Extract tar: {}", archive_error_string(a));
          return TarErrors::CANNOT_UNTAR_ARCHIVE;
        }
        if (r < ARCHIVE_OK)
          logger->warn("Extract tar: {}", archive_error_string(a));
      }
      r = archive_write_finish_entry(ext);
      if (r < ARCHIVE_WARN) {
        logger->error("Extract tar: {}", archive_error_string(a));
        return TarErrors::CANNOT_UNTAR_ARCHIVE;
      }
      if (r < ARCHIVE_OK)
        logger->warn("Extract tar: {}", archive_error_string(a));
    }
    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);

    return outcome::success();
  }

}  // namespace fc::common

OUTCOME_CPP_DEFINE_CATEGORY(fc::common, TarErrors, e) {
  using fc::common::TarErrors;
  switch (e) {
    case (TarErrors::CANNOT_CREATE_DIR):
      return "Tar Util: cannot create output dir";
    case (TarErrors::CANNOT_UNTAR_ARCHIVE):
      return "Tar Util: cannot untar archive";
    default:
      return "Tar Util: unknown error";
  }
}
