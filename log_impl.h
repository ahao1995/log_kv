#pragma once
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>

#include <array>
#include <cstdint>
#include <mutex>
#include <string_view>
#include <vector>

#include "file_sink.h"
#include "log.h"

namespace log_kv {
template <typename CharType, std::size_t Size>
struct string_literal : public std::array<CharType, Size + 1> {
  using base = std::array<CharType, Size + 1>;
  using value_type = typename base::value_type;
  using pointer = typename base::pointer;
  using const_pointer = typename base::const_pointer;
  using iterator = typename base::iterator;
  using const_iterator = typename base::const_iterator;
  using reference = typename base::const_pointer;
  using const_reference = typename base::const_pointer;
  constexpr string_literal() = default;
  constexpr string_literal(const CharType (&value)[Size + 1]) {
    // don't use std::copy_n here to support low version stdlibc++
    for (size_t i = 0; i < Size + 1; ++i) {
      (*this)[i] = value[i];
    }
  }
  constexpr string_literal(std::string_view s) {
    // don't use std::copy_n here to support low version stdlibc++
    for (size_t i = 0; i < Size; ++i) {
      (*this)[i] = s[i];
    }
  }
  constexpr std::size_t size() const { return Size; }
  constexpr bool empty() const { return !Size; }
  using base::begin;
  constexpr auto end() { return base::end() - 1; }
  constexpr auto end() const { return base::end() - 1; }
  using base::data;
  using base::operator[];
  using base::at;

 private:
  using base::cbegin;
  using base::cend;
  using base::rbegin;
  using base::rend;
};

template <typename CharType, std::size_t Size>
string_literal(const CharType (&value)[Size])
    -> string_literal<CharType, Size - 1>;

template <typename CharType, size_t Len1, size_t Len2>
decltype(auto) constexpr operator+(string_literal<CharType, Len1> str1,
                                   string_literal<CharType, Len2> str2) {
  auto ret = string_literal<CharType, Len1 + Len2>{};
  // don't use std::copy_n here to support low version stdlibc++
  for (size_t i = 0; i < Len1; ++i) {
    ret[i] = str1[i];
  }
  for (size_t i = Len1; i < Len1 + Len2; ++i) {
    ret[i] = str2[i - Len1];
  }
  return ret;
}
// https://github.com/MengRao/str
template <size_t SIZE>
class Str {
 public:
  static const int Size = SIZE;
  char s[SIZE];

  Str() {}
  Str(const char *p) { *this = *(const Str<SIZE> *)p; }

  char &operator[](int i) { return s[i]; }
  char operator[](int i) const { return s[i]; }

  template <typename T>
  void fromi(T num) {
    if constexpr (Size & 1) {
      s[Size - 1] = '0' + (num % 10);
      num /= 10;
    }
    switch (Size & -2) {
      case 18:
        *(uint16_t *)(s + 16) = *(uint16_t *)(digit_pairs + ((num % 100) << 1));
        num /= 100;
      case 16:
        *(uint16_t *)(s + 14) = *(uint16_t *)(digit_pairs + ((num % 100) << 1));
        num /= 100;
      case 14:
        *(uint16_t *)(s + 12) = *(uint16_t *)(digit_pairs + ((num % 100) << 1));
        num /= 100;
      case 12:
        *(uint16_t *)(s + 10) = *(uint16_t *)(digit_pairs + ((num % 100) << 1));
        num /= 100;
      case 10:
        *(uint16_t *)(s + 8) = *(uint16_t *)(digit_pairs + ((num % 100) << 1));
        num /= 100;
      case 8:
        *(uint16_t *)(s + 6) = *(uint16_t *)(digit_pairs + ((num % 100) << 1));
        num /= 100;
      case 6:
        *(uint16_t *)(s + 4) = *(uint16_t *)(digit_pairs + ((num % 100) << 1));
        num /= 100;
      case 4:
        *(uint16_t *)(s + 2) = *(uint16_t *)(digit_pairs + ((num % 100) << 1));
        num /= 100;
      case 2:
        *(uint16_t *)(s + 0) = *(uint16_t *)(digit_pairs + ((num % 100) << 1));
        num /= 100;
    }
  }

  static constexpr const char *digit_pairs =
      "00010203040506070809"
      "10111213141516171819"
      "20212223242526272829"
      "30313233343536373839"
      "40414243444546474849"
      "50515253545556575859"
      "60616263646566676869"
      "70717273747576777879"
      "80818283848586878889"
      "90919293949596979899";
};

struct source_loc {
  constexpr source_loc() = default;
  constexpr source_loc(const char *filename_in, int line_in,
                       const char *funcname_in)
      : filename{filename_in}, line{line_in}, funcname{funcname_in} {}

  constexpr bool empty() const noexcept { return line == 0; }
  const char *filename{nullptr};
  int line{0};
  const char *funcname{nullptr};
};
struct log_pattern {
  Str<4> year;
  char dash1 = '-';
  Str<2> month;
  char dash2 = '-';
  Str<2> day;
  char space = 'T';
  Str<2> hour;
  char colon1 = ':';
  Str<2> minute;
  char colon2 = ':';
  Str<2> second;
  char dot1 = '.';
  Str<6> microsec;
  Str<5> log_level;
};
class log_impl {
  using memory_buf_t = fmt::basic_memory_buffer<char>;

 public:
  static log_impl &get_log_impl() {
    static log_impl log_;
    return log_;
  }
  uint8_t get_log_level() { return level_; }
  void set_log_file(const char *file_name) {
    if (file_sink_.open(file_name)) {
      throw("file open failed");
    }
  }
  void set_log_level(uint8_t log_level) { level_ = log_level; }
  template <typename Func, typename... Args>
  void log_kv(source_loc loc, uint8_t loglevel, Func func, Args &&...arg) {
    constexpr auto fmt_string_literal = gen_fmt<Func, 0, Args...>(func);
    constexpr char enter[2] = "\n";
    constexpr auto fmt_string_literal_with_enter =
        fmt_string_literal + string_literal<char, 1>{enter};
    std::string_view fmt =
        std::string_view(fmt_string_literal_with_enter.data(),
                         fmt_string_literal_with_enter.size());
    {
      std::lock_guard<std::mutex> lock(mutex_);
      timeval tv;
      gettimeofday(&tv, NULL);
      tm local_tm;
      localtime_r(&(tv.tv_sec), &local_tm);
      set_log_pattern(loglevel, loc, local_tm, tv);
      log_buf_.clear();
      vformat_to(log_buf_, "ts={} tid={} level={} caller={}:{} func={} ",
                 fmt::basic_format_args(args_.data(), 6));
      vformat_to(log_buf_, fmt, fmt::make_format_args((arg)...));
      file_sink_.write(log_buf_.data(), log_buf_.size());
      file_sink_.flush();
    }
  }

 protected:
  log_impl() {
    args_.reserve(4096);
    args_.resize(6);
    set_arg<0>(fmt::string_view(log_pattern_.year.s,
                                26));  // year month day min sec mico_sec
    set_arg<1>(int(1));                // thread_id
    set_arg<2>(fmt::string_view(log_pattern_.log_level.s, 5));
    set_arg<3>(fmt::string_view());
    set_arg<4>(int(1));
    set_arg<5>(fmt::string_view());
  }

 private:
  template <size_t I, typename T>
  inline void set_arg(const T &arg) {
    args_[I] = fmt::detail::make_arg<fmt::format_context>(arg);
  }

  template <size_t I, typename T>
  inline void set_arg_val(const T &arg) {
    fmt::detail::value<fmt::format_context> &value_ =
        *(fmt::detail::value<fmt::format_context> *)&args_[I];
    value_ = fmt::detail::arg_mapper<fmt::format_context>().map(arg);
  }
  void vformat_to(memory_buf_t &out, fmt::string_view fmt,
                  fmt::format_args args) {
    fmt::detail::vformat_to(out, fmt, args);
  }

  template <typename Func, size_t Idx>
  static constexpr decltype(auto) gen_fmt(Func f) {
    return string_literal<char, 0>{};
  }

  template <typename Func, size_t Idx, typename Arg, typename... Args>
  static constexpr auto gen_fmt(Func func) {
    constexpr auto id =
        fmt::detail::mapped_type_constant<Arg, fmt::format_context>::value;
    if constexpr (id == fmt::detail::type::string_type ||
                  id == fmt::detail::type::cstring_type) {
      constexpr auto e = func()[Idx];
      constexpr int l = e.size();
      constexpr char f[7] = "=\"{}\" ";
      return string_literal<char, l>{e} + string_literal<char, 6>{f} +
             gen_fmt<Func, Idx + 1, Args...>(func);
    }
    else {
      constexpr auto e = func()[Idx];
      constexpr int l = e.size();
      constexpr char f[5] = "={} ";
      return string_literal<char, l>{e} + string_literal<char, 4>{f} +
             gen_fmt<Func, Idx + 1, Args...>(func);
    }
  }
  static inline const char *str_log_level(int level) {
    static const char *LOG_LEVEL[6] = {"FATAL", "ERROR", "WARN ",
                                       "INFO ", "DEBUG", "TRACE"};
    return LOG_LEVEL[level];
  }
  void set_log_pattern(uint8_t loglevel, source_loc loc, tm &local_tm,
                       timeval tv) {
    if (thread_sysid_ == -1) {
      thread_sysid_ = syscall(SYS_gettid);
    }
    log_pattern_.year.fromi(local_tm.tm_year + 1900);
    log_pattern_.month.fromi(local_tm.tm_mon + 1);
    log_pattern_.day.fromi(local_tm.tm_mday);
    log_pattern_.hour.fromi(local_tm.tm_hour);
    log_pattern_.minute.fromi(local_tm.tm_min);
    log_pattern_.second.fromi(local_tm.tm_sec);
    log_pattern_.microsec.fromi(tv.tv_usec);
    log_pattern_.log_level = str_log_level(loglevel);
    set_arg_val<1>(thread_sysid_);
    set_arg_val<3>(fmt::string_view(loc.filename));
    set_arg_val<4>(loc.line);
    set_arg_val<5>(fmt::string_view(loc.funcname));
  }

 private:
  uint8_t level_{5};
  file_sink file_sink_;
  std::vector<fmt::basic_format_arg<fmt::format_context>> args_;
  memory_buf_t log_buf_;
  log_pattern log_pattern_;
  static inline thread_local int thread_sysid_{-1};
  std::mutex mutex_;
};
}  // namespace log_kv