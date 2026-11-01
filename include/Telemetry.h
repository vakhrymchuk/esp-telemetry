#pragma once

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <functional>
#include <vector>
#include "Param.h"

class Telemetry {
public:
    using CommandCallback = std::function<void(const char* key, float value)>;

    void begin(const char* ssid = "Robot-AP", const char* password = "12345678");
    void send(const JsonDocument& doc);
    void loop();

    void onCommand(CommandCallback callback);

    // Привязать параметр для автоматического обновления из UI
    void bind(ParamBase& param);

    Telemetry& group(const char* name);

    // Добавить обычное значение
    template<typename T>
    Telemetry& add(const char* key, T value, bool editable = false) {
        const char* grp = currentGroup ? currentGroup : "_default";
        packet[grp][key]["v"] = value;
        if (editable) {
            packet[grp][key]["e"] = true;
        }
        return *this;
    }

    // Добавить Param (имя берётся автоматически)
    Telemetry& add(ParamBase& param, bool editable = false) {
        const char* grp = currentGroup ? currentGroup : "_default";
        packet[grp][param.getName()]["v"] = param.toFloat();
        if (editable) {
            packet[grp][param.getName()]["e"] = true;
        }
        return *this;
    }

    void send();
    void clear();

private:
    AsyncWebServer server{80};
    AsyncWebSocket ws{"/ws"};
    JsonDocument packet;
    const char* currentGroup = nullptr;
    CommandCallback cmdCallback = nullptr;
    std::vector<ParamBase*> boundParams;

    void handleWsMessage(uint8_t* data, size_t len);
    void onWsEvent(AsyncWebSocketClient* client, AwsEventType type, 
                   void* arg, uint8_t* data, size_t len);
    void serveHtml(AsyncWebServerRequest* request);
};
