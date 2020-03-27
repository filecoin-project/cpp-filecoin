#!/usr/bin/env bash

set -Eeo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")"

rust_sources_dir="../filecoin-ffi/rust"
release_sha1=$(cd $rust_sources_dir && git rev-parse HEAD)

if [ -e filecoin_ffi_commit_intalled ] && [ "$(cat filecoin_ffi_commit_intalled)" = "$release_sha1" ]; then
  (>&2 echo "filecoin-ffi is already installed")
  exit 0
fi

auth_header=()
if [ -n "${GITHUB_TOKEN}" ]; then
	auth_header=("-H" "Authorization: token ${GITHUB_TOKEN}")
fi

download_release_tarball() {
    __resultvar=$1
    __repo_name=$2
    __release_name="${__repo_name}-$(uname)"
    __release_tag="${release_sha1:0:16}"
    __release_tag_url="https://api.github.com/repos/filecoin-project/${__repo_name}/releases/tags/${__release_tag}"
    __release_response=""

    echo "acquiring release @ ${__release_tag}"

    __release_response=$(curl \
        --retry 3 \
        "${auth_header[@]}" \
        --location $__release_tag_url)



    if ! [ -x "$(command -v jq)" ]; then
        (>&2 echo 'Error: jq is not installed.')
        (>&2 echo 'Install jq to resolve this problem.')
        return 1
    fi

    __release_url=$(echo $__release_response | jq -r ".assets[] | select(.name | contains(\"${__release_name}\")) | .url")

    if [[ -z "$__release_url" ]]; then
        (>&2 echo "failed to download release (tag URL: ${__release_tag_url}, response: ${__release_response})")
        return 1
    fi

    __tar_path="/tmp/${__release_name}_$(basename ${__release_url}).tar.gz"

    __asset_url=""

    __asset_url=$(curl \
        --head \
        --retry 3 \
        --header "Accept:application/octet-stream" \
        "${auth_header[@]}"\
        --location \
        --output /dev/null \
        -w %{url_effective} \
        "$__release_url")


    curl --retry 3  --output "${__tar_path}" "$__asset_url"
    if [[ $? -ne "0" ]]; then
        (>&2 echo "failed to download release asset (tag URL: ${__release_tag_url}, asset URL: ${__asset_url})")
        return 1
    fi

    eval $__resultvar="'$__tar_path'"
}

build_from_source() {
    __library_name=$1
    __rust_sources_path=$2
    __repo_sha1_truncated="${release_sha1:0:16}"

    echo "building from source @ ${__repo_sha1_truncated}"

    if ! [ -x "$(command -v cargo)" ]; then
        (>&2 echo 'Error: cargo is not installed.')
        (>&2 echo 'Install Rust toolchain to resolve this problem.')
        exit 1
    fi

    if ! [ -x "$(command -v rustup)" ]; then
        (>&2 echo 'Error: rustup is not installed.')
        (>&2 echo 'Install Rust toolchain installer to resolve this problem.')
        exit 1
    fi

    pushd $__rust_sources_path

    cargo --version

    if [[ -f "./scripts/build-release.sh" ]]; then
        ./scripts/build-release.sh $__library_name $(cat rust-toolchain)
    else
        cargo build --release --all
    fi

    popd
}

mkdir -p include/filecoin-ffi
mkdir -p lib/pkgconfig

if [ "${FFI_BUILD_FROM_SOURCE}" != "1" ] && download_release_tarball tarball_path "filecoin-ffi"; then
    tmp_dir=$(mktemp -d)

    tar -C "$tmp_dir" -xzf "$tarball_path"

    find -L "${tmp_dir}" -type f -name filecoin.h -exec rsync --checksum "{}" ./include/filecoin-ffi \;
    find -L "${tmp_dir}" -type f -name libfilecoin.a -exec rsync --checksum "{}" ./lib \;
    find -L "${tmp_dir}" -type f -name filecoin.pc -exec rsync --checksum "{}" ./lib/pkgconfig \;

    (>&2 echo "successfully installed prebuilt libfilecoin")
else
    (>&2 echo "building libfilecoin from local sources (dir = ${rust_sources_dir})")

    build_from_source "filecoin" "${rust_sources_dir}"

    find -L "${rust_sources_dir}/target/release" -type f -name filecoin.h -exec rsync --checksum "{}" ./include/filecoin-ffi \;
    find -L "${rust_sources_dir}/target/release" -type f -name libfilecoin.a -exec rsync --checksum "{}" ./lib \;
    find -L "${rust_sources_dir}" -type f -name filecoin.pc -exec rsync --checksum "{}" ./lib/pkgconfig \;

    if [[ ! -f "./include/filecoin-ffi/filecoin.h" ]]; then
        (>&2 echo "failed to install filecoin.h")
        exit 1
    fi

    if [[ ! -f "./lib/libfilecoin.a" ]]; then
        (>&2 echo "failed to install libfilecoin.a")
        exit 1
    fi

    if [[ ! -f "./lib/pkgconfig/filecoin.pc" ]]; then
        (>&2 echo "failed to install filecoin.pc")
        exit 1
    fi

    (>&2 echo "successfully built and installed libfilecoin from source")
fi

echo "$release_sha1" > filecoin_ffi_commit_intalled
