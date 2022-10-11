#pragma once

#include <string>
#include <iostream>

#include <getopt.h>

#include "grpc/client.hpp"

struct config {
  // client params
  std::string host;
  std::string token;
  std::string text;
  std::string output_file_path;

  config();
  void parse(int ac, char **av);
  bool validate();
  void print_params() const;
  smartspeech::grpc::client::params get_grpc_client_params() const;
  smartspeech::grpc::synthesis::connection::params get_grpc_connection_params() const;

  static void print_help();
};