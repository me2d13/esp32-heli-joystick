#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <vector>

// Log levels
enum LogLevel {
  LOG_LEVEL_DEBUG = 0,  // Serial only, not stored
  LOG_LEVEL_INFO = 1,   // Serial + memory
  LOG_LEVEL_WARN = 2,   // Serial + memory
  LOG_LEVEL_ERROR = 3   // Serial + memory
};

// Log entry structure
struct LogEntry {
  unsigned long timestamp;  // Milliseconds since boot
  LogLevel level;
  String message;
};

class Logger {
public:
  Logger();
  
  // Initialize logger
  void begin(size_t maxEntries);
  
  // Log functions
  void debug(const char* message);
  void debug(const String& message);
  void info(const char* message);
  void info(const String& message);
  void warn(const char* message);
  void warn(const String& message);
  void error(const char* message);
  void error(const String& message);
  
  // Printf-style logging
  void debugf(const char* format, ...);
  void infof(const char* format, ...);
  void warnf(const char* format, ...);
  void errorf(const char* format, ...);
  
  // Get stored log entries
  const std::vector<LogEntry>& getEntries() const { return entries; }
  
  // Get log entries as JSON string
  String getEntriesJSON() const;
  
  // Clear all stored entries
  void clear();

private:
  std::vector<LogEntry> entries;
  size_t maxEntries;
  
  // Core logging function
  void log(LogLevel level, const String& message);
  
  // Format timestamp as H:MM:SS.mmm
  String formatTimestamp(unsigned long millis) const;
  
  // Get level name as string
  const char* getLevelName(LogLevel level) const;
};

// Global logger instance
extern Logger logger;

// Convenience macros
#define LOG_DEBUG(msg) logger.debug(msg)
#define LOG_INFO(msg) logger.info(msg)
#define LOG_WARN(msg) logger.warn(msg)
#define LOG_ERROR(msg) logger.error(msg)

#define LOG_DEBUGF(...) logger.debugf(__VA_ARGS__)
#define LOG_INFOF(...) logger.infof(__VA_ARGS__)
#define LOG_WARNF(...) logger.warnf(__VA_ARGS__)
#define LOG_ERRORF(...) logger.errorf(__VA_ARGS__)

#endif // LOGGER_H
