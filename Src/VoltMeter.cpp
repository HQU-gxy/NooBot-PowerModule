#include "main.h"

namespace VoltMeter
{
    extern "C"
    {
        extern ADC_HandleTypeDef hadc;
    }

    constexpr float MULTIPLIER = 3.3f * 10.43 / (1 << 12); // 12-bit ADC

    // Sample many times to get a good average
    constexpr auto N_SAMPLES = 10;
    static uint16_t adcBuf[N_SAMPLES]{0};

    void init()
    {
        HAL_ADC_Start_IT(&hadc);
    }

    void deinit()
    {
        HAL_ADC_Stop_IT(&hadc);
    }

    float getVoltage()
    {
        uint32_t temp = 0;
        for (auto val : adcBuf)
        {
            temp += val;
        }

        return temp / N_SAMPLES * MULTIPLIER;
    }
} // namespace VoltMeter

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    UNUSED(hadc);
    static uint8_t sampleIndex = 0;

    VoltMeter::adcBuf[sampleIndex++] = HAL_ADC_GetValue(hadc);
    if (sampleIndex >= VoltMeter::N_SAMPLES)
    {
        sampleIndex = 0;
    }
}
