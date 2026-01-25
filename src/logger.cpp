#include "logger.h"
#include "config.h"
#include <stdarg.h>

// Global logger instance
Logger logger;

Logger::Logger() : maxEntries(LOG_BUFFER_SIZE) {}

void Logger::begin(size_t maxEntries) {
  this->maxEntries = maxEntries;
  entries.reserve(maxEntries);
}

void Logger::log(LogLevel level, const String& message) {
  unsigned long now = millis();
  String timestamp = formatTimestamp(now);
  String levelName = getLevelName(level);
  
  // Always output to Serial
  Serial.printf("[%s] %s: %s\n", timestamp.c_str(), levelName, message.c_str());
  
  // Store in memory only if level >= INFO
  if (level >= LOG_LEVEL_INFO) {
    LogEntry entry;
    entry.timestamp = now;
    entry.level = level;
    entry.message = message;
    
    // Add to circular buffer
    if (entries.size() >= maxEntries) {
      entries.erase(entries.begin());  // Remove oldest entry
    }
    entries.push_back(entry);
  }
}

String Logger::formatTimestamp(unsigned long millis) const {
  unsigned long totalSeconds = millis / 1000;
  unsigned long ms = millis % 1000;
  unsigned long seconds = totalSeconds % 60;
  unsigned long minutes = (totalSeconds / 60) % 60;
  unsigned long hours = totalSeconds / 3600;
  
  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%lu:%02lu:%02lu.%03lu", hours, minutes, seconds, ms);
  return String(buffer);
}

const char* Logger::getLevelName(LogLevel level) const {
  switch (level) {
    case LOG_LEVEL_DEBUG: return "DEBUG";
    case LOG_LEVEL_INFO:  return "INFO ";
    case LOG_LEVEL_WARN:  return "WARN ";
    case LOG_LEVEL_ERROR: return "ERROR";
    default:              return "?????";
  }
}

void Logger::debug(const char* message) {
  log(LOG_LEVEL_DEBUG, String(message));
}

void Logger::debug(const String& message) {
  log(LOG_LEVEL_DEBUG, message);
}

void Logger::info(const char* message) {
  log(LOG_LEVEL_INFO, String(message));
}

void Logger::info(const String& message) {
  log(LOG_LEVEL_INFO, message);
}

void Logger::warn(const char* message) {
  log(LOG_LEVEL_WARN, String(message));
}

void Logger::warn(const String& message) {
  log(LOG_LEVEL_WARN, message);
}

void Logger::error(const char* message) {
  log(LOG_LEVEL_ERROR, String(message));
}

void Logger::error(const String& message) {
  log(LOG_LEVEL_ERROR, message);
}

void Logger::debugf(const char* format, ...) {
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  log(LOG_LEVEL_DEBUG, String(buffer));
}

void Logger::infof(const char* format, ...) {
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  log(LOG_LEVEL_INFO, String(buffer));
}

void Logger::warnf(const char* format, ...) {
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  log(LOG_LEVEL_WARN, String(buffer));
}

void Logger::errorf(const char* format, ...) {
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  log(LOG_LEVEL_ERROR, String(buffer));
}

String Logger::getEntriesJSON() const {
  String json = "[";
  
  for (size_t i = 0; i < entries.size(); i++) {
    if (i > 0) json += ",";
    
    const LogEntry& entry = entries[i];
    String timestamp = formatTimestamp(entry.timestamp);
    String levelName = getLevelName(entry.level);
    
    // Escape quotes in message
    String escapedMessage = entry.message;
    escapedMessage.replace("\"", "\\\"");
    
    json += "{";
    json += "\"timestamp\":\"" + timestamp + "\",";
    json += "\"level\":\"" + String(levelName) + "\",";
    json += "\"message\":\"" + escapedMessage + "\"";
    json += "}";
  }
  
  json += "]";
  return json;
}

void Logger::clear() {
  entries.clear();
}
