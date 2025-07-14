#include "config.hpp"

config::config()
    : host("smartspeech.sber.ru")
    , audio_encoding_flag(1)
    , sample_rate(48000)
    , model("general")
    , hypotheses_count(1)
    , enable_partial_results(true)
    , enable_multi_utterance(false)
    , no_speech_timeout_ms(7000)
    , max_speech_timeout_ms(2000) {}

void config::parse(int ac, char **av) {
  while (true) {
    static struct option long_options[] = {{"host", required_argument, NULL, 'a'},
                                           {"token", required_argument, NULL, 't'},
                                           {"file", required_argument, NULL, 'f'},
                                           {"rate", required_argument, NULL, 'r'},
                                           {0, 0, 0, 0}};
    int option_index = 0;
    int c = getopt_long(ac, av, "a:t:f:r:h", long_options, &option_index);
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
        case 'f':
          audio_file_path = std::string(optarg);
          break;
        case 'r':
          sample_rate = std::atoi(optarg);
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
  if (audio_file_path.empty()) {
    return false;
  }
  return true;
}

void config::print_params() const {
  std::cout << "grpc server address: " << host << '\n';
  std::cout << "audio file path: " << audio_file_path << '\n';
}

smartspeech::grpc::client::params config::get_grpc_client_params() const {
  smartspeech::grpc::client::params p{};
  p.host = this->host;
  p.token = this->token;
  return p;
}

smartspeech::grpc::recognition::connection::params config::get_grpc_connection_params() const {
  smartspeech::grpc::recognition::connection::params p{};
  p.audio_encoding_flag = this->audio_encoding_flag;
  p.sample_rate = this->sample_rate;
  p.model = this->model;
  p.hypotheses_count = this->hypotheses_count;
  p.enable_partial_results = this->enable_partial_results;
  p.enable_multi_utterance = this->enable_multi_utterance;
  p.no_speech_timeout_ms = this->no_speech_timeout_ms;
  p.max_speech_timeout_ms = this->max_speech_timeout_ms;

  return p;
}

void config::print_help() {
  std::cout << "usage: recognition -t <access_token> -f <pcm audio file> -r <sample rate>\n";
}