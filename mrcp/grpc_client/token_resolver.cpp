#include "token_resolver.hpp"

#include <chrono>
#include <iostream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <random>
#include <thread>

namespace {
std::string get_uuid() {
  static std::random_device dev;
  static std::mt19937 rng(dev());

  std::uniform_int_distribution<int> dist(0, 15);

  const char *v = "0123456789abcdef";
  const bool dash[] = {0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0};

  std::string res;
  for (int i = 0; i < 16; i++) {
    if (dash[i])
      res += "-";
    res += v[dist(rng)];
    res += v[dist(rng)];
  }
  return res;
}
}  // namespace

namespace smartspeech {

void token_resolver::global_init() {
  static bool f = false;
  if (!f) {
    f = true;
    curl_global_init(CURL_GLOBAL_NOTHING);
  }
}

void token_resolver::global_cleanup() {
  static bool f = false;
  if (!f) {
    f = true;
    curl_global_cleanup();
  }
}

token_resolver::token_resolver(const std::string &sm_url, const std::string &client_id, const std::string &secret,
                               const std::string &scope)
    : keep_(true)
    , token_("")
    , url_(sm_url)
    , uuid_(get_uuid())
    , client_id_(client_id)
    , secret_(secret)
    , scope_(scope)
    , curl_(nullptr) {
  worker_thread_ = std::thread(&token_resolver::worker_proc, this);
}

token_resolver::~token_resolver() {
  keep_ = false;
  cv_.notify_one();
  if (worker_thread_.joinable()) {
    worker_thread_.join();
  }
}

static size_t on_received(void *received_data, size_t size, size_t count, void *userdata) {
  size_t realsize = size * count;

  if (realsize) {
    auto *secret = static_cast<std::string *>(userdata);
    secret->append(static_cast<char *>(received_data), realsize);
  }

  return realsize;
}

std::string token_resolver::get_token() {
  std::lock_guard<std::mutex> l(m_);
  return token_;
}

std::pair<std::string, size_t> token_resolver::fetch_token() {
  std::pair<std::string, size_t> result{"", 15000};
  std::string smartmarket_response;

  curl_ = curl_easy_init();

  curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, on_received);
  curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &smartmarket_response);

  struct curl_slist *hs = NULL;
  hs = curl_slist_append(hs, "Content-Type: application/x-www-form-urlencoded");
  std::string rquid = std::string{"RqUID: "} + uuid_;
  hs = curl_slist_append(hs, rquid.c_str());

  curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, hs);

  curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt(curl_, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
  curl_easy_setopt(curl_, CURLOPT_USERNAME, client_id_.c_str());
  curl_easy_setopt(curl_, CURLOPT_PASSWORD, secret_.c_str());

  curl_easy_setopt(curl_, CURLOPT_POST, 1L);
  std::string scope_opt = std::string{"scope="} + scope_;
  curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, scope_opt.c_str());
  curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 3L);

  std::string secret_url = url_ + "/api/v2/oauth";
  curl_easy_setopt(curl_, CURLOPT_URL, secret_url.c_str());

  CURLcode res = curl_easy_perform(curl_);
  if (res == CURLE_OK && !smartmarket_response.empty()) {
    auto json = nlohmann::json::parse(smartmarket_response);
    if (json.count("access_token")) {
      result.first = json["access_token"];
    } else {
      std::cerr << "smartspecch: get token err: can't find token in the SmartMarket API response\n";
    }

    if (json.count("expires_at")) {
      size_t expire_at_unix_ms = json["expires_at"];
      auto now_unix_ms =
          std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
              .count();
      result.second = expire_at_unix_ms - now_unix_ms - 1000;
    }

  } else {
    std::cerr << "smartspeech: get token err: " << curl_easy_strerror(res) << std::endl;
  }
  curl_slist_free_all(hs);
  curl_easy_cleanup(curl_);

  if (!result.first.empty()) {
    std::cout << "[smartspeech:token_fetcher]: token successfully received. Expires in " << result.second << "ms.\n";
  }

  return result;
}

void token_resolver::worker_proc() {
  std::unique_lock<std::mutex> l(m_);
  auto fetch_result = fetch_token();
  auto expires_in = fetch_result.second;

  token_ = fetch_result.first;
  l.unlock();

  while (keep_) {
    cv_.wait_for(l, std::chrono::milliseconds(expires_in));
    if (!keep_) {
      break;
    }
    fetch_result = fetch_token();
    token_ = fetch_result.first;
    expires_in = fetch_result.second;
  }
}

}  // namespace smartspeech
