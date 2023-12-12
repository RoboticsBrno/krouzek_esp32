#include <Arduino.h>
#include <Wire.h>

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

#define USE_TWI_DIRECTLY 1
#define I2C_ADDR 0xF0

static int gIter = 0;
static i2c_t gI2C;

void setup() {
    Serial.begin(115200);

    gI2C.sda = digitalPinToPinName(SDA);
    gI2C.scl = digitalPinToPinName(SCL);
    gI2C.isMaster = 1;

    i2c_init(&gI2C);


#if USE_TWI_DIRECTLY

#else
    Wire.begin();

    for(uint16_t addr = 0xE0; addr <= 0xFE; addr += 2) {
        Serial.printf("Addr 0x%02x\n", addr);
        uint8_t res = Wire.requestFrom(addr, 1, 0, 1, 1);

        Serial.printf("   %d\n", res);

        /*Wire.beginTransmission(addr);
        Wire.write(0);
        Wire.endTransmission(0);

        char res = 0;
        auto status = i2c_master_read(&Wire._i2c, addr << 1, &res, 1);

        Serial.printf("   %d %d\n", res, status);*/
    }
#endif
}

void loop() {
    //write_header(0x00, 4);
    //write_scalar(millis());


#if USE_TWI_DIRECTLY
    uint8_t measureCmd[] = {
        0x0,
        0x51,
    };

    i2c_master_write(&gI2C, I2C_ADDR << 1, measureCmd, sizeof(measureCmd), 1);

    delay(65);

    uint8_t res_hi = 0;
    uint8_t res_lo = 0;

    uint8_t reg_num = 2;
    i2c_master_write(&gI2C, I2C_ADDR << 1, &reg_num, 1, 0);
    i2c_master_read(&gI2C, I2C_ADDR << 1, &res_hi, 1);

    reg_num = 3;
    i2c_master_write(&gI2C, I2C_ADDR << 1, &reg_num, 1, 0);
    i2c_master_read(&gI2C, I2C_ADDR << 1, &res_lo, 1);

#else
    Wire.beginTransmission(I2C_ADDR);
    Wire.write(0);
    Wire.write(0x51); // Start measurement in cm
    Wire.endTransmission(1);

    delay(65);

    Wire.requestFrom(I2C_ADDR, 1, 2, 1, 1);
    uint8_t res_hi = Wire.read();
    Wire.requestFrom(I2C_ADDR, 1, 3, 1, 1);
    uint8_t res_lo = Wire.read();

#endif

    Serial.printf("%d %d %d\n", res_hi, res_lo, (res_hi << 8) | res_lo);


    delay(100);
    //digitalWrite(PD4, gIter%2);
    gIter++;
}
