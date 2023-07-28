#include "../log.h"
#include <thread>
#include <unordered_map>
#include <vector>

int main() {
  SET_LOG_FILE("./log/test.log");
  SET_LOG_LEVEL(LOG_INFO);
  std::vector<std::thread> theads;
  for (int i = 0; i < 4; i++) {
    theads.push_back(std::thread([]() {
      for (int i = 0; i < 10000; i++) {
        LOG_KV(LOG_INFO, ("test", 1));
      }
    }));
  }
  for (auto &t : theads) {
    t.join();
  }
  LOG_KV(LOG_INFO, ("key_1", 1), ("key_2", 1.0000), ("key_3", "m"));
  std::string str = "test_str";
  std::string_view str_view = "test_string_view";
  const char *str_char_pointer = "test_char_pointer";
  const char str_char_array[] = "test_char_array";
  LOG_KV(LOG_INFO, ("key_1", str), ("key_2", std::string("str")),
         ("key_3", std::string_view("key3_str_view")), ("key_4", str_view),
         ("key_5", str_char_pointer), ("key_6", str_char_array));
  LOG_KV(LOG_INFO, ("key_nan", NAN),
         ("key_inf", std::numeric_limits<double>::infinity()));
  auto vec = std::vector<int>{1, 2, 3};
  auto map = std::unordered_map<int, double>{{1, 2.1}, {2, 3.3}};
  LOG_KV(LOG_INFO, ("key_vec", vec), ("key_map", map));
}