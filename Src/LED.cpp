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

    /**
     * @brief Initialize the LED timer.
     */
    void init()
    {
        // The 1kHz timer for LED blinking
        HAL_TIM_Base_Start_IT(&htim14);
    }

    /**
     * @brief Deinitialize the LED timer and turn off all LEDs.
     */
    void deinit()
    {
        HAL_TIM_Base_Stop_IT(&htim14);
        for (auto pin : ledPins)
        {
            HAL_GPIO_WritePin(GPIOA, pin, GPIO_PIN_RESET);
        }
    }

    /**
     * @brief Set the current blinking pattern.
     * 
     * @param pattern The blinking pattern to set.
     */
    void setCurrentPattern(const BlinkPattern &pattern)
    {
        currentPattern = pattern;
    }

    /**
     * @brief Reset the pattern index to 0.
     */
    void resetIndex()
    {
        patternIndex = 0;
    }

    /**
     * @brief Handle the LED blinking logic.
     * 
     * @note This function is called in the timer interrupt callback.
     */
    void handleBlink()
    {
        // No pattern set, do nothing
        if (currentPattern.pattern.size() == 0)
        {
            return;
        }
        // Reset the index if it is out of bounds
        if (patternIndex >= currentPattern.pattern.size())
        {
            resetIndex();
        }

        auto ledMask = currentPattern.pattern[patternIndex].first;
        auto duration = currentPattern.pattern[patternIndex].second;
        // Caution: All LEDs are connected to GPIOA
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

/**
 * @brief The callback function for the timer interrupt.
 * 
 * @param htim The HAL timer handle.
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    UNUSED(htim);
    LED::handleBlink();
}
