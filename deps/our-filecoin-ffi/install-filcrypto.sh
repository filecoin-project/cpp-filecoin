#!/usr/bin/env bash
# shellcheck disable=SC2155 enable=require-variable-braces

set -Eeo pipefail

ffi_dir=`realpath ../filecoin-ffi`
release_sha1=$(git -C $ffi_dir rev-parse HEAD)

if [ -e filecoin_ffi_commit_installed ] && [ "$(cat filecoin_ffi_commit_installed)" = "$release_sha1" ]; then
  exit 0
fi

if ! [ -x "$(command -v jq)" ]; then
  (>&2 echo 'Error: jq is not installed.')
  (>&2 echo 'Install jq to resolve this problem.')
  exit 1
fi

main() {
    mkdir -p include/filecoin-ffi
    mkdir -p lib/pkgconfig
    ${ffi_dir}/install-filcrypto

    find -L "${ffi_dir}" -type f -name filcrypto.h -exec rsync --checksum "{}" ./include/filecoin-ffi \;
    find -L "${ffi_dir}" -type f -name libfilcrypto.a -exec rsync --checksum "{}" ./lib \;
    find -L "${ffi_dir}" -type f -name filcrypto.pc -exec rsync --checksum "{}" ./lib/pkgconfig \;
    echo "$release_sha1" > filecoin_ffi_commit_installed
}

main "$@"; exit
