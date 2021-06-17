#include "EspMQTTClient.h"
#include <driver/adc.h>

static EspMQTTClient gMqttClient(
    "Anthrophobia",
    "Ku1ata2elvA",
    "192.168.0.126", // MQTT Broker server ip
    "mojeesp",       // Can be omitted if not needed
    "ZelenyKun",     // Can be omitted if not needed
    "<in setup()>"   // Client name that UNIQUELY identify your device
);

static unsigned long gNextPublish = ULONG_MAX;
static char gMac[17];

void setup()
{
  uint64_t macInt;
  esp_efuse_mac_get_default((uint8_t *)&macInt);
  snprintf(gMac, sizeof(gMac), "%016llx", macInt);

  gMqttClient.setMqttClientName(gMac);

  adc1_config_width(ADC_WIDTH_BIT_12);
}

void onConnectionEstablished()
{
  printf("Established!\n");

  gMqttClient.subscribe("mytopic/#", [](const String &topicStr, const String &payload){
    printf("%s: %s", topicStr.c_str(), payload.c_str());
  });

  gNextPublish = 0;
}

void loop()
{
  gMqttClient.loop();

  if (gNextPublish < millis())
  {
    char buf[256];
    snprintf(buf, sizeof(buf), "%s: hal: %d\n", gMac, hall_sensor_read());

    gMqttClient.publish("mytopic/temperature", buf, true);
    gNextPublish = millis() + 100;
  }
}
