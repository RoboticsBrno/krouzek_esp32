#pragma once

#include <Arduino.h>


class MillisTimer {
public:
    MillisTimer(): m_periodMs(0), m_expireAt(0) {}
    MillisTimer(unsigned long period): m_periodMs(period), m_expireAt(0) {}

    void start() {
        start(m_periodMs);
    }

    void start(unsigned long expireIn) {
        m_periodMs = expireIn;
        m_expireAt = millis() + expireIn;
    }

    bool isStarted() const { return m_expireAt != 0; }

    bool expired() {
        if(m_expireAt != 0 && millis() > m_expireAt) {
            m_expireAt = 0;
            return true;
        }
        return false;
    }

private:
    unsigned long m_periodMs;
    unsigned long m_expireAt;
};
