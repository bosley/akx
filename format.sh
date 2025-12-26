#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

COMMON_EXCLUDE_DIRS=(
  ".git"
  ".cache"
  "build"
  "cmake-build-*"
  "third_party"
  "external"
  "extern"
  "Extern"
  "vendor"
  "node_modules"
)

LINUX_EXCLUDE_DIRS=()

DARWIN_EXCLUDE_DIRS=()

OTHER_EXCLUDE_DIRS=()

HEADER_EXTENSIONS=(
  "h"
  "hpp"
  "hxx"
  "hh"
  "H"
  "HPP"
)

SOURCE_EXTENSIONS=(
  "c"
  "cpp"
  "cxx"
  "cc"
  "C"
  "CPP"
)

check_clang_format() {
  if ! command -v clang-format >/dev/null 2>&1; then
    echo "clang-format not found" >&2
    exit 1
  fi
}

detect_cpus() {
  case "$(uname -s)" in
    Linux) nproc ;;
    Darwin) sysctl -n hw.ncpu ;;
    *) echo 1 ;;
  esac
}

build_exclude_clause() {
  local clause=""
  local dir

  for dir in "${COMMON_EXCLUDE_DIRS[@]}"; do
    if [ -n "$clause" ]; then
      clause="$clause -o "
    fi
    clause="$clause-path \"*/$dir/*\""
  done

  if [ -n "$clause" ]; then
    echo "\\( $clause \\) -prune -o"
  fi
}

build_extension_pattern() {
  local pattern=""
  local ext

  for ext in "${HEADER_EXTENSIONS[@]}" "${SOURCE_EXTENSIONS[@]}"; do
    if [ -n "$pattern" ]; then
      pattern="$pattern\\|"
    fi
    pattern="$pattern$ext"
  done

  echo "$pattern"
}

build_header_name_clause() {
  local clause=""
  local ext

  for ext in "${HEADER_EXTENSIONS[@]}"; do
    if [ -n "$clause" ]; then
      clause="$clause -o "
    fi
    clause="$clause-name \"*.$ext\""
  done

  echo "\\( $clause \\)"
}

build_source_name_clause() {
  local clause=""
  local ext

  for ext in "${SOURCE_EXTENSIONS[@]}"; do
    if [ -n "$clause" ]; then
      clause="$clause -o "
    fi
    clause="$clause-name \"*.$ext\""
  done

  echo "\\( $clause \\)"
}

format_linux() {
  local exclude_clause ext_pattern
  exclude_clause=$(build_exclude_clause)
  ext_pattern=$(build_extension_pattern)

  eval find "$ROOT" \
    "$exclude_clause" \
    -type f -regex "\".*\\.\\($ext_pattern\\)\"" -print0 \| \
    xargs -0 -n 50 -P "$1" clang-format -style=file -i
}

format_darwin() {
  local exclude_clause header_clause source_clause
  exclude_clause=$(build_exclude_clause)
  header_clause=$(build_header_name_clause)
  source_clause=$(build_source_name_clause)

  eval find "$ROOT" \
    "$exclude_clause" \
    "$header_clause" -type f -print0 \| \
    xargs -0 -n 50 -P "$1" clang-format -style=file -i

  eval find "$ROOT" \
    "$exclude_clause" \
    "$source_clause" -type f -print0 \| \
    xargs -0 -n 50 -P "$1" clang-format -style=file -i
}

format_other() {
  local exclude_clause header_clause source_clause
  exclude_clause=$(build_exclude_clause)
  header_clause=$(build_header_name_clause)
  source_clause=$(build_source_name_clause)

  eval find "$ROOT" \
    "$exclude_clause" \
    "$header_clause" -type f -print0 \| \
    xargs -0 -n 50 clang-format -style=file -i

  eval find "$ROOT" \
    "$exclude_clause" \
    "$source_clause" -type f -print0 \| \
    xargs -0 -n 50 clang-format -style=file -i
}

main() {
  check_clang_format

  local cpus
  cpus="$(detect_cpus)"

  case "$(uname -s)" in
    Linux)
      format_linux "$cpus"
      ;;
    Darwin)
      format_darwin "$cpus"
      ;;
    *)
      format_other "$cpus"
      ;;
  esac
}

main
