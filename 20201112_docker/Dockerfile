# docker build -t test .
# docker run -t -i --volume $(pwd)/output:/output test

FROM python:3.8.6-slim-buster

RUN pip install platformio && \
    pip install kikit

RUN pio platform install espressif32@~1.12.4


WORKDIR /app

ADD . .

VOLUME /output

CMD platformio run && \
    cp .pio/build/esp32dev/firmware.* /output/
