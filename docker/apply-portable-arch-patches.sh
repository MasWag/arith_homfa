#!/usr/bin/env bash

set -euo pipefail

if [[ "$#" -ne 2 ]]; then
    echo "Usage: $0 <repository-root> <patch-directory>" >&2
    exit 2
fi

repository_root="$(realpath "$1")"
patch_directory="$(realpath "$2")"

repositories=(
    "thirdparty/TFHEpp"
    "thirdparty/TFHEpp/thirdparties/hexl/hexl"
    "examples/blood_glucose/homfa-experiment"
    "examples/blood_glucose/homfa-experiment/trgsw-enc-dec/TFHEpp"
    "examples/blood_glucose/homfa-experiment/trgsw-enc-dec/TFHEpp/thirdparties/hexl/hexl"
)

patches=(
    "tfhepp-portable-arch.patch"
    "hexl-portable-arch.patch"
    "homfa-experiment-portable-arch.patch"
    "homfa-tfhepp-portable-arch.patch"
    "hexl-portable-arch.patch"
)

for index in "${!repositories[@]}"; do
    git -C "$repository_root/${repositories[$index]}" \
        apply --check --index --unidiff-zero "$patch_directory/${patches[$index]}"
done

for index in "${!repositories[@]}"; do
    git -C "$repository_root/${repositories[$index]}" \
        apply --index --unidiff-zero "$patch_directory/${patches[$index]}"
done
