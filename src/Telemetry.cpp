#include "Telemetry.h"
#include <WiFi.h>

static const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Robot Telemetry</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: #1a1a2e;
            color: #eee;
            min-height: 100vh;
            padding: 20px;
        }
        h1 { text-align: center; margin-bottom: 30px; color: #0ff; }
        .status { text-align: center; margin-bottom: 20px; }
        .status span {
            display: inline-block;
            width: 12px; height: 12px;
            border-radius: 50%;
            margin-right: 8px;
            background: #f00;
        }
        .status.connected span { background: #0f0; }
        .container { max-width: 900px; margin: 0 auto; }
        .group { margin-bottom: 25px; }
        .group-title {
            font-size: 14px;
            color: #888;
            text-transform: uppercase;
            letter-spacing: 1px;
            margin-bottom: 10px;
            padding-left: 5px;
        }
        .grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(140px, 1fr));
            gap: 12px;
        }
        .card {
            background: #16213e;
            border-radius: 12px;
            padding: 15px;
            text-align: center;
        }
        .card.editable { background: #1e3a5f; cursor: pointer; }
        .card.editable:hover { background: #254a73; }
        .label { font-size: 12px; color: #888; margin-bottom: 5px; }
        .value { font-size: 32px; font-weight: bold; color: #0ff; }
        .card.editable .value { color: #ff0; }
        .edit-input {
            width: 80px;
            font-size: 24px;
            text-align: center;
            background: #0a1628;
            border: 2px solid #0ff;
            color: #ff0;
            border-radius: 6px;
            padding: 5px;
        }
        .edit-input:focus { outline: none; border-color: #ff0; }
    </style>
</head>
<body>
    <h1>🤖 Robot Telemetry</h1>
    <div class="status" id="status"><span></span>Disconnected</div>
    <div class="container" id="container"></div>
    <script>
        const ws = new WebSocket(`ws://${location.host}/ws`);
        const status = document.getElementById('status');
        const container = document.getElementById('container');
        const groups = {};
        const order = ['gyroscope', 'physics', '_default'];
        const editableKeys = {};

        ws.onopen = () => {
            status.className = 'status connected';
            status.innerHTML = '<span></span>Connected';
        };

        ws.onclose = () => {
            status.className = 'status';
            status.innerHTML = '<span></span>Disconnected';
        };

        function getGroup(name) {
            if (!groups[name]) {
                const div = document.createElement('div');
                div.className = 'group';
                div.id = 'group-' + name;
                const title = name === '_default' ? 'Other' : name;
                div.innerHTML = `<div class="group-title">${title}</div><div class="grid" id="grid-${name}"></div>`;
                
                let inserted = false;
                for (const g of order) {
                    if (g === name) { container.appendChild(div); inserted = true; break; }
                    if (groups[g] && !inserted) continue;
                    if (groups[g]) { container.insertBefore(div, groups[g].parentElement); inserted = true; break; }
                }
                if (!inserted) container.appendChild(div);
                
                groups[name] = { grid: document.getElementById('grid-' + name), cards: {} };
            }
            return groups[name];
        }

        function createCard(group, groupName, key, editable) {
            const card = document.createElement('div');
            card.className = 'card' + (editable ? ' editable' : '');
            card.innerHTML = `<div class="label">${key}</div><div class="value" id="val-${groupName}-${key}">--</div>`;
            
            if (editable) {
                editableKeys[`${groupName}-${key}`] = true;
                card.onclick = () => {
                    const valEl = document.getElementById(`val-${groupName}-${key}`);
                    const current = valEl.textContent;
                    valEl.innerHTML = `<input type="number" class="edit-input" value="${current}" id="input-${groupName}-${key}">`;
                    const input = document.getElementById(`input-${groupName}-${key}`);
                    input.focus();
                    input.select();
                    
                    const submit = () => {
                        const newVal = parseFloat(input.value);
                        if (!isNaN(newVal)) {
                            ws.send(JSON.stringify({ key: key, value: newVal }));
                        }
                        valEl.textContent = input.value;
                    };
                    
                    input.onblur = submit;
                    input.onkeydown = (e) => {
                        if (e.key === 'Enter') submit();
                        if (e.key === 'Escape') valEl.textContent = current;
                    };
                };
            }
            
            group.grid.appendChild(card);
            group.cards[key] = true;
        }

        function formatValue(v) {
            return typeof v === 'number' ? (Number.isInteger(v) ? v : v.toFixed(2)) : v;
        }

        ws.onmessage = (e) => {
            const data = JSON.parse(e.data);
            for (const [groupName, values] of Object.entries(data)) {
                const group = getGroup(groupName);
                for (const [key, item] of Object.entries(values)) {
                    const value = item.v !== undefined ? item.v : item;
                    const editable = item.e === true;
                    
                    if (!group.cards[key]) {
                        createCard(group, groupName, key, editable);
                    }
                    
                    const valEl = document.getElementById(`val-${groupName}-${key}`);
                    if (valEl && !valEl.querySelector('input')) {
                        valEl.textContent = formatValue(value);
                    }
                }
            }
        };
    </script>
</body>
</html>
)rawliteral";

void Telemetry::begin(const char* ssid, const char* password) {
    WiFi.softAP(ssid, password);
    Serial.print("AP started. Connect to WiFi: ");
    Serial.println(ssid);
    Serial.print("Open in browser: http://");
    Serial.println(WiFi.softAPIP());

    ws.onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client,
                      AwsEventType type, void* arg, uint8_t* data, size_t len) {
        onWsEvent(client, type, arg, data, len);
    });
    server.addHandler(&ws);

    server.on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
        serveHtml(request);
    });

    server.begin();
}

void Telemetry::onCommand(CommandCallback callback) {
    cmdCallback = callback;
}

void Telemetry::bind(ParamBase& param) {
    boundParams.push_back(&param);
}

void Telemetry::send(const JsonDocument& doc) {
    String json;
    serializeJson(doc, json);
    ws.textAll(json);
}

Telemetry& Telemetry::group(const char* name) {
    currentGroup = name;
    return *this;
}

void Telemetry::send() {
    String json;
    serializeJson(packet, json);
    ws.textAll(json);
    clear();
}

void Telemetry::clear() {
    packet.clear();
    currentGroup = nullptr;
}

void Telemetry::loop() {
    ws.cleanupClients();
}

void Telemetry::handleWsMessage(uint8_t* data, size_t len) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, data, len);
    if (err) return;

    const char* key = doc["key"];
    float value = doc["value"];
    
    if (!key) return;

    // Сначала ищем среди привязанных параметров
    for (auto* param : boundParams) {
        if (strcmp(param->getName(), key) == 0) {
            param->setFromFloat(value);
            Serial.printf("Param %s = %.2f\n", key, value);
            return;
        }
    }

    // Если не нашли - вызываем callback
    if (cmdCallback) {
        Serial.printf("Command: %s = %.2f\n", key, value);
        cmdCallback(key, value);
    }
}

void Telemetry::onWsEvent(AsyncWebSocketClient* client, AwsEventType type,
                          void* arg, uint8_t* data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("Client #%u connected\n", client->id());
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("Client #%u disconnected\n", client->id());
    } else if (type == WS_EVT_DATA) {
        handleWsMessage(data, len);
    }
}

void Telemetry::serveHtml(AsyncWebServerRequest* request) {
    request->send(200, "text/html", HTML_PAGE);
}
