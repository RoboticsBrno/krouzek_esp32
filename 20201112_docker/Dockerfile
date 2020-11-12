FROM python:3.7-slim

RUN pip3 install platformio

RUN pio platform install espressif32@~1.12.4

WORKDIR /app

ADD . .

VOLUME /output

CMD pio run && cp -a .pio/build/esp32dev/firmware.* output/
