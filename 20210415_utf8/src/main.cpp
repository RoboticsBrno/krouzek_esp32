#include <Arduino.h>

void setup() {
    delay(1000);

    const char* unicode = "🤦🏼‍♂️ žluťoučký kůň";

    printf("\n%s.length === %d\n", unicode, strlen(unicode));
}

void loop() {}
