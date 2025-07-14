#pragma once

#include <string>
#include <iostream>

#include <getopt.h>

#include "grpc/client.hpp"

struct config {
  // client params
  std::string host;
  std::string token;
  // sample audio
  std::string audio_file_path;
  // session params
  uint16_t audio_encoding_flag;  // 1=pcm_s16le; 2=opus
  int32_t sample_rate;
  std::string model;
  int32_t hypotheses_count;
  bool enable_multi_utterance;
  bool enable_partial_results;
  size_t no_speech_timeout_ms;
  size_t max_speech_timeout_ms;

  config();
  void parse(int ac, char **av);
  bool validate();
  void print_params() const;
  smartspeech::grpc::client::params get_grpc_client_params() const;
  smartspeech::grpc::recognition::connection::params get_grpc_connection_params() const;

  static void print_help();
};