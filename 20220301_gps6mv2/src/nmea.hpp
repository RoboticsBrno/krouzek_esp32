#pragma once

#include <driver/uart.h>

#include <functional>
#include <array>
#include <string.h>
#include <algorithm>


class NmeaLineBuffer {
public:
  static const size_t BufferSize = 82 + 1; // 1 for NULL byte

  NmeaLineBuffer() : m_pos(0) {}

  size_t free() const { return BufferSize - m_pos - 1; }

  size_t size() const { return BufferSize - 1; }

  const char *begin() const { return m_buf.begin(); }
  char *begin() { return m_buf.begin(); }

  char *position() { return m_buf.begin() + m_pos; }

  void reset(char *source = NULL, size_t len = 0) {
    m_pos = len;
    if(len > 0) {
      memmove(m_buf.data(), source, len);
    }
  }

  void advance(int delta) {
    m_pos = delta;
  }

private:
  std::array<char, BufferSize> m_buf;
  size_t m_pos;
};

enum NmeaMessageType : uint8_t {
  Unknown = 0,
  GGA = 1, // Time, position and fix type data
  RMC = 2, // Time, date, position, speed, course
};

enum NmeaFixIndicator : uint8_t {
  NoFix = 0,
  GpsFix = 1,
  DifferencialGpsFix = 2,
};

struct NmeaMessage {
  const char *raw; // valid only until next readMessage call
  NmeaMessageType type;

  struct GpsTime {
    float seconds;
    uint8_t hours;
    uint8_t minutes;
  };

  struct GpsDate {
    uint8_t day;
    uint8_t month;
    uint8_t year; // last two digits only
  };

  struct GGAMessage {
    GpsTime time;

    float latitude;
    float longitude;
    NmeaFixIndicator fix_indicator;
    uint8_t used_satellites;
    float HDOP;
    float altitude_meters;
    float geoid_separation;
  };

  struct RMCMessage {
    GpsTime time;
    GpsDate date;
    char status;
    float latitude;
    float longitude;
    float speed_over_ground;
    float course_over_ground;
    float magnetic_variation;
  };

  union {
    GGAMessage gga;
    RMCMessage rmc;
  };
};

class NmeaParser {
public:
  NmeaParser(uart_port_t uart_num) : m_uart(uart_num), m_currentBuffer(0) {

  }

  NmeaMessage readMessage() {
    auto *line = readline();
    NmeaMessage msg = { };
    msg.raw = line;

    char *typeStr = line + 3; // skip $GP
    char *fieldsStr = typeStr + 4;

    if(memcmp(typeStr, "GGA", 3) == 0) {
      msg.type = NmeaMessageType::GGA;
      auto& g = msg.gga;

      iterateOverFields(fieldsStr, [&](size_t idx, const char *field) {
          switch(idx) {
            case 0: parseTime(field, g.time); break;
            case 1: parseLatitude(field, g.latitude); break;
            case 2: if(*field == 'S') g.latitude *= -1; break;
            case 3: parseLongitude(field, g.longitude); break;
            case 4: if(*field == 'W') g.longitude *= -1; break;
            case 5: sscanf(field, "%hhu", (uint8_t*)&g.fix_indicator); break;
            case 6: sscanf(field, "%hhu", &g.used_satellites); break;
            case 7: sscanf(field, "%f", &g.HDOP); break;
            case 8: sscanf(field, "%f", &g.altitude_meters); break;
            case 9: if(*field != 'M') g.altitude_meters = 0; break; // prevent unit mismatch, not sure if anything else than 'M' can be emitted
            case 10: sscanf(field, "%f", &g.geoid_separation); break;
            case 11: if(*field != 'M') g.geoid_separation = 0; break; // prevent unit mismatch, not sure if anything else than 'M' can be emitted
          }
      });
    } else if(memcmp(typeStr, "RMC", 3) == 0) {
      msg.type = NmeaMessageType::RMC;
      auto& r = msg.rmc;

      iterateOverFields(fieldsStr, [&](size_t idx, const char *field) {
          switch(idx) {
            case 0: parseTime(field, r.time); break;
            case 1: r.status = *field; break;
            case 2: parseLatitude(field, r.latitude); break;
            case 3: if(*field == 'S') r.latitude *= -1; break;
            case 4: parseLongitude(field, r.longitude); break;
            case 5: if(*field == 'W') r.longitude *= -1; break;
            case 6: sscanf(field, "%f", &r.speed_over_ground); break;
            case 7: sscanf(field, "%f", &r.course_over_ground); break;
            case 8: parseDate(field, r.date); break;
            case 9: sscanf(field, "%f", &r.magnetic_variation); break;
            case 10: if(*field == 'W') r.magnetic_variation *= -1; break;
          }
      });
    }

    return msg;
  }

private:

  void iterateOverFields(char *line, const std::function<void(size_t idx, const char *field)>& callback) {
    size_t idx = 0;
    char *pos = line;

    for(;;) {
      char *next = strchr(pos, ',');
      if(next != NULL) {
        *next = 0;
      }

      callback(idx++, pos);

      if(next == NULL) {
        return;
      }

      *next = ',';
      pos = next + 1;
    }
  }

  void parseTime(const char *src, NmeaMessage::GpsTime& dest) {
    sscanf(src, "%2hhu%2hhu%f", &dest.hours, &dest.minutes, &dest.seconds);
  }

  void parseDate(const char *src, NmeaMessage::GpsDate& dest) {
    sscanf(src, "%2hhu%2hhu%2hhu", &dest.day, &dest.month, &dest.year);
  }

  void parseLatitude(const char *src, float& dest) {
    int deg;
    float minutes;
    sscanf(src, "%2d%f", &deg, &minutes);
    dest = minutes / 60 + deg;
  }

  void parseLongitude(const char *src, float& dest) {
    int deg;
    float minutes;
    sscanf(src, "%3d%f", &deg, &minutes);
    dest = minutes / 60 + deg;
  }

  char *readline() {
    auto& buf = m_buffers[m_currentBuffer];
    while(true) {
      if(buf.free() == 0) {
        buf.reset();
        continue;
      }

      const int readCount = uart_read_bytes(m_uart, buf.position(), buf.free(), 0);
      if(readCount <= 0) {
        vTaskDelay(1);
        continue;
      }

      auto *readBytesEnd = buf.position() + readCount;
      auto *endlItr = std::find(buf.position(), readBytesEnd, '\n');
      if(endlItr == readBytesEnd) {
        buf.advance(readCount);
        continue;
      }

      const int leftoverCount = readBytesEnd - (endlItr + 1);
      if(strncmp(buf.begin(), "$GP", 3) == 0) {
        m_currentBuffer = !m_currentBuffer;

        m_buffers[m_currentBuffer].reset(endlItr + 1, leftoverCount);

        *(endlItr+1) = 0;
        return buf.begin();
      }

      buf.reset(endlItr + 1, leftoverCount);
    }
  }

  uart_port_t m_uart;

  int m_currentBuffer;
  NmeaLineBuffer m_buffers[2];
};
