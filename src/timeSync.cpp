#include <Arduino.h>
#include "timesync.h"

void NTPSync::begin(const char* tz,
                      const char* servers[],
                      size_t serverCount,
                      uint32_t retrySeconds)
{
    _servers = servers;
    _serverCount = serverCount;
    _retrySeconds = retrySeconds;
    _tzString = tz;

    // 1. Bootzeit setzen
    struct timeval tv = { .tv_sec = 0, .tv_usec = 0 };
    settimeofday(&tv, nullptr);

    // 2. Kalenderzeit aktivieren (ohne NTP)
    configTime(0, 0, nullptr, nullptr, nullptr);

    // 3. Zeitzone setzen
    setenv("TZ", _tzString, 1);
    tzset();
    
    // 4. SNTP Callback
    sntp_set_time_sync_notification_cb([](struct timeval *tv){
        NTPSync::instance()._synced = true;
    });

    // 5. ersten Server starten
    startNtp();
}

void NTPSync::startNtp()
{
    if (_serverCount == 0) return;

    const char* s = _servers[_currentServer];
    configTime(0, 0, s);

    // WICHTIG: TZ erneut setzen, weil configTime sie überschreibt
    setenv("TZ", _tzString, 1);
    tzset();

    _nextRetry = millis() + _retrySeconds * 1000;
}

void NTPSync::scheduleRetry()
{
    if (_serverCount == 0) return;
    if (_synced) return;

    if (millis() > _nextRetry)
    {
        _currentServer = (_currentServer + 1) % _serverCount;
        startNtp();
    }
}

bool NTPSync::isSynced() const {
    return _synced;
}

bool NTPSync::hasRealTime() const {
    return time(nullptr) > 1577836800;
}

String NTPSync::GetDateTime() const
{
    struct tm ti;
    getLocalTime(&ti);

    char buf[32];
    strftime(buf, sizeof(buf), "%H:%M:%S %d.%m.%y:", &ti);

    String prefix;

    if (isSynced())
        prefix = "";
    else if (hasRealTime())
        prefix = "(RTC)";
    else
        prefix = "(BT)";

    return prefix + String(buf);
}
    

String NTPSync::GetTime() const
{
    struct tm ti;
    getLocalTime(&ti);  

    char buf[32];
    strftime(buf, sizeof(buf), "%H:%M:%S", &ti);

    String prefix;

    if (isSynced())
        prefix = "";
    else if (hasRealTime())
        prefix = "(RTC)";
    else
        prefix = "(BT)";

    return prefix + String(buf);
}


NTPSync& NTPSync::instance() {
    static NTPSync inst;
    return inst;
}
