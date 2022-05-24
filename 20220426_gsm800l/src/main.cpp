#include <Arduino.h>
#include <HardwareSerial.h>

#include "SIM800L.h"


static HardwareSerial gSerialGsm(1);
static SIM800L gGsm;

void setup() {
    gSerialGsm.begin(115200, SERIAL_8N1, 25, 26);

    if(gGsm.begin(gSerialGsm)) {
        printf("GSM initialized\n");
    } else {
        printf("failed to initialize GSM\n");
        return;
    }

    String oper = gGsm.serviceProvider();
    
    printf("Operator: %s\n", oper.c_str());

    
    gGsm.tcpConnect("example.com", 80);

    while(!gGsm.tcpStatus()) {
        printf("Connecting....\n");
        delay(300);
    }
    printf("Connected!\n");

    gGsm.tcpSend("GET / HTTP/1.1\r\nHost: example.com\r\n\r\n");

    while(gGsm.tcpAvailable() <= 0) {
        printf("Waiting for TCP response...\n");
        delay(300);
    }

    char buf[256] = {};
    while(true) {
        const int avail = gGsm.tcpAvailable();
        if(avail <= 0) {
            break;
        }

        printf("%d\n", avail);

        const int chunk = std::min(avail, (int)sizeof(buf));
        buf[0] = 0;
        gGsm.tcpRead(buf, chunk);
        printf("%*s\n", chunk, buf);
    }

}


void loop() {
    uint8_t time[6] = { };
    if(gGsm.GSMTime(time)) {
        printf("got time: %d-%d-%d %d:%d:%d\n", time[0], time[1], time[2], time[3], time[4], time[5]);
    }


    delay(1000);
}
