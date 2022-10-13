#!/bin/bash

has_param() {
  local term="$1"
  shift
  for arg; do
    if [[ $arg == "$term" ]]; then
      return 0
    fi
  done
  return 1
}


set -eux
set -o pipefail

cd /src
if has_param '--drop-cache' "$@"; then
    bazel --output_user_root='/bazel-root' clean --expunge
fi
bazel --output_user_root='/bazel-root' build --cxxopt='-std=c++17' //:plugins
bazel --output_user_root='/bazel-root' run '@bazel_compile_commands//:gen_compile_commands.sh'

mkdir -p output && chmod o+w output
cp bazel-bin/plugin/smartspeech-recognition-plugin.so ./output
cp bazel-bin/plugin/smartspeech-synthesis-plugin.so ./output
