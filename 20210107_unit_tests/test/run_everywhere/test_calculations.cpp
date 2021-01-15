// https://docs.platformio.org/en/latest/plus/unit-testing.html#api
#include <unity.h>

#include "calculations.h"


static void test_factorial() {
    TEST_ASSERT_EQUAL(calcFactorial(3), 6);
    TEST_ASSERT_EQUAL(calcFactorial(5), 120);
    TEST_ASSERT_EQUAL(calcFactorial(10), 3628800);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(calcFactorial(5), 123, "factorial is equal and it is not supposed to be");
}

static void test_fibonacci() {
    TEST_ASSERT_EQUAL(calcFibonnaci(0), 0);
    TEST_ASSERT_EQUAL(calcFibonnaci(1), 1);
    TEST_ASSERT_EQUAL(calcFibonnaci(2), 1);
    TEST_ASSERT_EQUAL(calcFibonnaci(9), 34);
}

static void process() {
    UNITY_BEGIN();
    RUN_TEST(test_factorial);
    RUN_TEST(test_fibonacci);
    UNITY_END();
}

#ifdef ARDUINO

#include <Arduino.h>
void setup() {
    process();
}

void loop() {}

#else

int main(int argc, char **argv) {
    process();
    return 0;
}

#endif
