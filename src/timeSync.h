
#pragma once
#include <time.h>
#include <sys/time.h>
#include "esp_sntp.h"

class NTPSync {
public:
    void begin(const char* tz,
               const char* servers[],
               size_t serverCount,
               uint32_t retrySeconds = 10);

    bool isSynced() const;
    bool hasRealTime() const;
    String GetTime() const;
    String GetDateTime() const;
    void scheduleRetry();

    static NTPSync& instance();

private:
    void startNtp();

    const char** _servers = nullptr;
    size_t _serverCount = 0;
    size_t _currentServer = 0;
    uint32_t _retrySeconds = 10;
    const char* _tzString = nullptr;

    volatile bool _synced = false;
    unsigned long _nextRetry = 0;

};
