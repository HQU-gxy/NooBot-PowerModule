#include "main.h"
#include "LED.h"
#include <array>

namespace LED
{
    extern "C"
    {
        extern TIM_HandleTypeDef htim14;
    }

    constexpr uint8_t N_LEDS = 4;
    constexpr std::array<uint16_t, N_LEDS> ledPins{LED1_Pin, LED2_Pin, LED3_Pin, LED4_Pin};

    static BlinkPattern currentPattern;
    static uint32_t lastPatternTime = 0;
    static uint16_t patternIndex = 0;

    void init()
    {
        // The 1kHz timer for LED blinking
        HAL_TIM_Base_Start_IT(&htim14);
    }

    void deinit()
    {
        HAL_TIM_Base_Stop_IT(&htim14);
        for (auto pin : ledPins)
        {
            HAL_GPIO_WritePin(GPIOA, pin, GPIO_PIN_RESET);
        }
    }

    void setCurrentPattern(const BlinkPattern &pattern)
    {
        currentPattern = pattern;
    }

    void resetIndex()
    {
        patternIndex = 0;
    }

    void handleBlink()
    {
        // Caution: All LEDs are connected to GPIOA
        if (currentPattern.pattern.size() == 0)
        {
            return;
        }
        if (patternIndex >= currentPattern.pattern.size())
        {
            resetIndex();
        }

        auto ledMask = currentPattern.pattern[patternIndex].first;
        auto duration = currentPattern.pattern[patternIndex].second;
        for (auto i = 0; i < N_LEDS; i++)
        {
            HAL_GPIO_WritePin(GPIOA, ledPins[i], (ledMask & (1 << i)) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        }

        if (HAL_GetTick() - lastPatternTime >= duration)
        {
            lastPatternTime = HAL_GetTick();
            // Check if the pattern is finished
            if (++patternIndex >= currentPattern.pattern.size())
            {
                if (currentPattern.oneshot)
                {
                    currentPattern.pattern.clear();
                }
                else
                {
                    patternIndex = 0;
                }
            }
        }
    }

} // namespace LED

// A timer with a period of 1 millisecond is used to blink the LEDs
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    UNUSED(htim);
    LED::handleBlink();
}
