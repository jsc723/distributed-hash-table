#pragma once
#include "headers.hpp"

enum class LogLevel {
  CRITICAL = 0,
  IMPORTANT = 1,
  INFO = 2,
  DEBUG = 3,
};

struct Logger {
    void set_log_level(LogLevel level) {
      cur_log_level = level;
    }
    LogLevel get_log_level() {
      return cur_log_level;
    }
    void log(LogLevel level, const char *prefix, const char *format, ...) {
      va_list arglist;
      va_start( arglist, format );
      vlog(level, prefix, format, arglist);
      va_end( arglist );
    }
    void vlog(LogLevel level, const char *prefix, const char *format, va_list arglist) {
      if (int(level) > (int)cur_log_level) return;
      switch(level) {
        case LogLevel::CRITICAL :
            fprintf(stderr, "[CRITICAL]");
            break;
        case LogLevel::IMPORTANT :
            fprintf(stderr, "[IMPORTANT]");
            break;
        case LogLevel::INFO :
            fprintf(stderr, "[INFO]");
            break;
        case LogLevel::DEBUG :
            fprintf(stderr, "[DEBUG]");
            break;
      }
      fprintf(stderr, "%s", prefix);
      vfprintf( stderr, format, arglist);
      fprintf(stderr, "\n");
    }
  private:
    LogLevel cur_log_level;
};
extern Logger logger;

class Logable {
public:
  void critical(const char *format, ...) {
        va_list arglist; va_start( arglist, format );
        vlog(LogLevel::CRITICAL, format, arglist);
        va_end( arglist );
    }
    void important(const char *format, ...) {
        va_list arglist; va_start( arglist, format );
        vlog(LogLevel::IMPORTANT, format, arglist);
        va_end( arglist );
    }
    void info(const char *format, ...) {
        va_list arglist; va_start( arglist, format );
        vlog(LogLevel::INFO, format, arglist);
        va_end( arglist );
    }
    void debug(const char *format, ...) {
        va_list arglist; va_start( arglist, format );
        vlog(LogLevel::DEBUG, format, arglist);
        va_end( arglist );
    }
    
    void log(LogLevel level, const char *format, ...) {
        va_list arglist; va_start( arglist, format );
        vlog(level, format, arglist);
        va_end( arglist );
    }
    virtual void vlog(LogLevel level, const char *format, va_list args) = 0;
};