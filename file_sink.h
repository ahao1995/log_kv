#pragma once
#include <cstdio>
#include <string>
#include <sys/stat.h>

class file_sink {
public:
  bool open(const char *file_name) {
    create_dir(dir_name(file_name).c_str());
    file_ = ::fopen(file_name, "a+");
    return file_ == nullptr;
  }
  void close() {
    if (file_ != nullptr) {
      fclose(file_);
    }
  }
  void write(const char *data, int len) { std::fwrite(data, 1, len, file_); }
  void flush() { std::fflush(file_); }

private:
  static bool mkdir_(const char *path) {
    return ::mkdir(path, mode_t(0755)) == 0;
  }
  std::string dir_name(const char *cur_path) {
    std::string path = cur_path;
    auto pos = path.find_last_of("/");
    return pos != std::string::npos ? path.substr(0, pos) : std::string{};
  }
  bool path_exists(const char *filename) {
    struct stat buffer;
    return (::stat(filename, &buffer) == 0);
  }
  bool create_dir(const char *cur_path) {
    std::string path = cur_path;
    if (path_exists(path.c_str())) {
      return true;
    }
    if (path.empty()) {
      return false;
    }
    size_t search_offset = 0;
    do {
      auto token_pos = path.find_first_of("/", search_offset);
      if (token_pos == std::string::npos) {
        token_pos = path.size();
      }
      auto subdir = path.substr(0, token_pos);
      if (!subdir.empty() && !path_exists(subdir.c_str()) &&
          !mkdir_(subdir.c_str())) {
        return false;
      }
      search_offset = token_pos + 1;
    } while (search_offset < path.size());

    return true;
  }

private:
  std::FILE *file_{nullptr};
};