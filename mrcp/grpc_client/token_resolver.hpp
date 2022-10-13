#pragma once

#include <curl/curl.h>

#include <atomic>
#include <condition_variable>
#include <string>
#include <thread>

namespace smartspeech {
class token_resolver {
 public:
  token_resolver(const std::string &sm_url, const std::string &client_id, const std::string &secret,
                 const std::string &scope);
  ~token_resolver();

  std::string get_token();

  static void global_init();
  static void global_cleanup();

 private:
  void worker_proc();
  std::pair<std::string, size_t> fetch_token();

 private:
  std::thread worker_thread_;
  std::condition_variable cv_;
  std::mutex m_;

  std::atomic_bool keep_;
  std::string token_;

  std::string url_;
  std::string uuid_;
  std::string client_id_;
  std::string secret_;
  std::string scope_;

  CURL *curl_;
};
}  // namespace smartspeech
