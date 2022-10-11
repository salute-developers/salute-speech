#include "config.hpp"

config::config()
    : host("smartspeech.sber.ru")
    {}

void config::parse(int ac, char **av) {
  while (true) {
    static struct option long_options[] = {{"host", required_argument, NULL, 'a'},
                                           {"token", required_argument, NULL, 't'},
                                           {"input", required_argument, NULL, 'i'},
                                           {"output", required_argument, NULL, 'o'},
                                           {0, 0, 0, 0}};
    int option_index = 0;
    int c = getopt_long(ac, av, "a:t:i:o:h", long_options, &option_index);
    if (c == -1) {
      break;
    }
    try {
      switch (c) {
        case 'a':
          host = std::string(optarg);
          break;
        case 't':
          token = std::string(optarg);
          break;
        case 'i':
          text = std::string(optarg);
          break;
        case 'o':
          output_file_path = std::string(optarg);
          break;
        case 'h':
          print_help();
          break;
        default:
          break;
      }
    } catch (const std::exception &e) {
      std::cerr << "cmd parsing error: " << e.what() << "\n";
    }
  }
}

bool config::validate() {
  if (host.empty()) {
    return false;
  }
  if (token.empty()) {
    return false;
  }
  if (text.empty()) {
    return false;
  }
  return true;
}

void config::print_params() const {
  std::cout << "grpc server address: " << host << '\n';
  std::cout << "text to synth: " << text << '\n';
  std::cout << "output file: " << output_file_path << '\n';
}

smartspeech::grpc::client::params config::get_grpc_client_params() const {
  smartspeech::grpc::client::params p{};
  p.host = this->host;
  p.token = this->token;
  return p;
}

smartspeech::grpc::synthesis::connection::params config::get_grpc_connection_params() const {
  smartspeech::grpc::synthesis::connection::params p{};
  p.text = this->text;
  return p;
}

void config::print_help() {
  std::cout << "usage: synthesis -t <access_token> --text <text to synth> -o <output_file>\n";
}