#include <Arduino.h>

static void write_header(uint8_t cmd, uint8_t data_len) {
    uint8_t buf[] = {
        0xFF,
        cmd,
        data_len
    };
    Serial.write((char*)buf, sizeof(buf));
}

template<typename T>
static void write_scalar(const T data) {
    Serial.write((char*)&data, sizeof(T));
}


static int gIter = 0;

static void gpioIsr() {
    write_header(0x02, 1);
    write_scalar(uint8_t(digitalRead(PD3)));
}

void setup() {
    Serial.begin(115200);

    // PA1 a PA2 jsou piny, na který je na dev desce zapojený krystal == nejdou použít
    // Vyčítání GPIO stavu z PA1 nefunguje ani se správně nastaveným pinMode
    
    pinMode(PD3, INPUT_PULLUP);

    attachInterrupt(PD3, GPIO_Mode_IPU, gpioIsr, EXTI_Mode_Interrupt, EXTI_Trigger_Rising_Falling);
}

void loop() {
    write_header(0x00, 4);
    write_scalar(millis());

    //write_header(0x02, 1);
    //write_scalar(uint8_t(digitalRead(PD3)));


    delay(100);
    //digitalWrite(PD4, gIter%2);
    gIter++;
}
