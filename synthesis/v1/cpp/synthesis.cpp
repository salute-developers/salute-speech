#include <fstream>

#include "config.hpp"
#include "grpc/client.hpp"

int main(int ac, char **av) {
  config conf{};

  conf.parse(ac, av);
  if (!conf.validate()) {
    config::print_help();
    return 1;
  }

  conf.print_params();

  std::atomic_bool finish = false;
  std::ofstream ofs(conf.output_file_path, std::ios::binary);

  // start client (grpc channel)
  smartspeech::grpc::client grpc_client(conf.get_grpc_client_params());

  // start connection (grpc stub)
  auto synth_connection = grpc_client.start_synth_connection(conf.get_grpc_connection_params(),
                                                             [&ofs, &finish](const smartspeech::grpc::synthesis::result &r) {
                                                               ofs.write((char *)r.buffer.data(), r.buffer.size());
                                                               if (r.end) {
                                                                 finish = true;
                                                               }
                                                             });

  while (!finish) {
    std::this_thread::sleep_for(std::chrono::seconds(3));
  }
  ofs.close();

  return 0;
}