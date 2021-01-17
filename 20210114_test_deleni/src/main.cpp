#include <Arduino.h>

template <typename T>
static void testDiv(const char *name)
{
    const size_t size = 8000;
    std::vector<T> data(size);
    for (size_t i = 0; i < size; ++i)
    {
        data[i] = esp_random();
    }

    const T div = esp_random();

    const auto start = xTaskGetTickCount();
    for (int x = 0; x < 50; ++x)
    {
        for (size_t i = 0; i < size; ++i)
        {
            data[i] /= div;
        }
    }
    printf("%8s (%d) divide took %4dms\n", name, sizeof(T), xTaskGetTickCount() - start);
    vTaskDelay(10);
}

template <typename T>
static void testMult(const char *name)
{
    const size_t size = 8000;
    std::vector<T> data(size);
    for (size_t i = 0; i < size; ++i)
    {
        data[i] = esp_random();
    }

    const T div = esp_random();

    const auto start = xTaskGetTickCount();
    for (int x = 0; x < 50; ++x)
    {
        for (size_t i = 0; i < size; ++i)
        {
            data[i] *= div;
        }
    }
    printf("%8s (%d) multiply took %dms\n", name, sizeof(T), xTaskGetTickCount() - start);
    vTaskDelay(10);
}

void setup()
{
    testDiv<uint8_t>("uint8");
    testDiv<uint16_t>("uint16");
    testDiv<uint32_t>("uint32");
    testDiv<uint64_t>("uint64");
    testDiv<float>("float");
    testDiv<double>("double");

    printf("\n");

    testMult<uint8_t>("uint8");
    testMult<uint16_t>("uint16");
    testMult<uint32_t>("uint32");
    testMult<uint64_t>("uint64");
    testMult<float>("float");
    testMult<double>("double");
}

void loop()
{
}
