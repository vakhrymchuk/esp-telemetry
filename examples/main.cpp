#include <Arduino.h>
#include "Telemetry.h"
#include "Param.h"

Param<int> targetSpeed{"target", 100};
int actualSpeed = 0;
Telemetry telemetry;

void setup() {
    Serial.begin(115200);
    telemetry.begin("Robot-AP", "12345678");
    telemetry.bind(targetSpeed);
    Serial.println("Initialized");
}

void loop() {
    actualSpeed = targetSpeed + random(-10, 11);

    Serial.printf("Target:%d Actual:%d\n", (int)targetSpeed, actualSpeed);

    telemetry
        .group("physics")
            .add(targetSpeed, true)
            .add("actual", actualSpeed)
            .add("accel", random(0, 50))
        .add("battery", 87)
        .add("uptime", millis() / 1000)
        .send();

    telemetry.loop();
    delay(200);
}
