#include <Arduino.h>
#include <driver/adc.h>

void setup() {
  // put your setup code here, to run once:v

  adc1_config_width(ADC_WIDTH_BIT_12);
}

void loop() {
  printf("%d\n", hall_sensor_read());
  delay(200);
}
