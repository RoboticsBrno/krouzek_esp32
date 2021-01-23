#include <Arduino.h>

template <typename T>
static TickType_t testDiv(const char *name, TickType_t base)
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

    auto duration = xTaskGetTickCount() - start;
    if (base == 0)
        base = duration;

    printf("%8s (%d) divide took %4dms - %.2fx\n", name, sizeof(T), duration, float(duration) / float(base));

    vTaskDelay(10);

    return duration;
}

template <typename T>
static TickType_t testMult(const char *name, TickType_t base)
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

    auto duration = xTaskGetTickCount() - start;
    if (base == 0)
        base = duration;

    printf("%8s (%d) multiply took %4dms - %.2fx\n", name, sizeof(T), duration, float(duration) / float(base));
    vTaskDelay(10);

    return duration;
}

void setup()
{
    vTaskDelay(500);

    printf("\n\n");

    auto base = testDiv<uint8_t>("uint8", 0);
    testDiv<uint16_t>("uint16", base);
    testDiv<uint32_t>("uint32", base);
    testDiv<uint64_t>("uint64", base);
    testDiv<float>("float", base);
    testDiv<double>("double", base);

    printf("\n");

    base = testMult<uint8_t>("uint8", 0);
    testMult<uint16_t>("uint16", base);
    testMult<uint32_t>("uint32", base);
    testMult<uint64_t>("uint64", base);
    testMult<float>("float", base);
    testMult<double>("double", base);
}

void loop()
{
}
