#pragma once
#include "log_impl.h"
#include "log_macro.h"

typedef enum {
  LOG_FATAL,
  LOG_ERROR,
  LOG_WARNING,
  LOG_INFO,
  LOG_DEBUG,
  LOG_TRACE
} LOG_LEVEL;

#define SOURCE_LOC                                                             \
  log_kv::source_loc { __FILE__, __LINE__, __FUNCTION__ }

#define SET_LOG_FILE(file) log_kv::log_impl::get_log_impl().set_log_file(file)
#define SET_LOG_LEVEL(level)                                                   \
  log_kv::log_impl::get_log_impl().set_log_level(level)

#define LOG_KV(level, ...)                                                     \
  do {                                                                         \
    if (log_kv::log_impl::get_log_impl().get_log_level() >= level) {            \
      log_kv::log_impl::get_log_impl().log_kv(                                 \
          SOURCE_LOC, level,                                                   \
          []() {                                                               \
            return MAKE_FMT_ARRAY(PP_GET_ARG_COUNT(__VA_ARGS__), __VA_ARGS__); \
          },                                                                   \
          MAKE_LOG_ARGS(PP_GET_ARG_COUNT(__VA_ARGS__), __VA_ARGS__));          \
    }                                                                          \
  } while (0)
