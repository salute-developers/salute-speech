#include <fstream>

#include "grpc/client.hpp"
#include "config.hpp"

int main(int ac, char **av) {
  config conf{};

  conf.parse(ac, av);
  if (!conf.validate()) {
    config::print_help();
    return 1;
  }

  conf.print_params();

  // start client (grpc channel)
  smartspeech::grpc::client grpc_client(conf.get_grpc_client_params());

  // start connection (grpc stub)
  auto recognition_connection = grpc_client.start_recognition_connection(
      conf.get_grpc_connection_params(), [](const smartspeech::grpc::recognition::result &r) {
        // on result callback
        std::cout << "[" << ((r.eou) ? "eou" : "partial") << "]: words: \"" << r.words << "\"; normalized: \""
                  << r.normalized << "\"\n";
      });

  // read file and feed connection
  static const size_t chunk_size = 1600;
  std::vector<uint8_t> audio_chunk(chunk_size, 0);
  std::ifstream ifs(conf.audio_file_path, std::ios::binary);
  while (ifs) {
    ifs.read((char *)audio_chunk.data(), audio_chunk.size());
    auto count = ifs.gcount();
    if (!count) {
      break;
    }
    recognition_connection->feed(audio_chunk.data(), count);
  }
  // signal that no more data available
  recognition_connection->writes_done();

  // waiting recognition result
  std::this_thread::sleep_for(std::chrono::seconds(3));

  return 0;
}