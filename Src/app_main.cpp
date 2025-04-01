#include "main.h"
#include "LED.h"
#include "VoltMeter.h"
#include <utility>

static const LED::BlinkPattern startupPattern = {
    .oneshot = true,
    .pattern = {
        {0b0000, 100},
        {0b0001, 300},
        {0b0011, 300},
        {0b0111, 300},
        {0b1111, 300},
    },
};

static const LED::BlinkPattern confirmShutdownPattern = {
    .oneshot = false,
    .pattern = {
        {0b1111, 300},
        {0b0000, 300},
    },
};

static const LED::BlinkPattern shutdownPattern = {
    .oneshot = true,
    .pattern = {
        {0b1111, 300},
        {0b0111, 300},
        {0b0011, 300},
        {0b0001, 300},
        {0b0000, 100},
    },
};

inline bool buttonPressed()
{
  return !HAL_GPIO_ReadPin(Button_GPIO_Port, Button_Pin);
}

inline void enablePower(bool enable)
{
  HAL_GPIO_WritePin(PWREN_GPIO_Port, PWREN_Pin, enable ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

inline uint8_t getBatteryPercentage(float voltage)
{
  // 6S 3.7V LiPo battery
  constexpr auto N_CELLS = 6;
  constexpr auto MIN_VOLTAGE = 3.0f;
  constexpr auto MAX_VOLTAGE = 4.2f;

  if (voltage < N_CELLS * MIN_VOLTAGE)
    return 0;
  else if (voltage > N_CELLS * MAX_VOLTAGE)
    return 100;
  else
    return static_cast<uint8_t>((voltage - N_CELLS * MIN_VOLTAGE) / (N_CELLS * (MAX_VOLTAGE - MIN_VOLTAGE)) * 100);
}

inline LED::BlinkPattern voltageToPattern(float voltage)
{
  auto percentage = getBatteryPercentage(voltage);
  LED::BlinkPattern result{.oneshot = false};

  if (percentage > 87)
    result.pattern = {
        {0b1111, 500},
    };
  else if (percentage > 75)
    result.pattern = {
        {0b1111, 500},
        {0b0111, 500},
    };
  else if (percentage > 62)
    result.pattern = {
        {0b0111, 500},
    };
  else if (percentage > 50)
    result.pattern = {
        {0b0111, 500},
        {0b0011, 500},
    };
  else if (percentage > 37)
    result.pattern = {
        {0b0011, 500},
    };
  else if (percentage > 25)
    result.pattern = {
        {0b0011, 500},
        {0b0001, 500},
    };
  else if (percentage > 12)
    result.pattern = {
        {0b0001, 500},
    };
  else
    result.pattern = {
        {0b0001, 500},
        {0b0000, 500},
    };

  return result;
}

void app_main()
{
  static uint32_t startPressTime;
  bool startedPressing = false;

  constexpr auto TIMEOUT = 3000;
  constexpr auto LONG_PRESS_LENGTH = 1000;

  while (true)
  {
    LED::init();
    VoltMeter::init();

    bool running = false;

    // Wait for the button to be released
    while (buttonPressed())
    {
      HAL_Delay(100);
    }
    auto idleBeginTime = HAL_GetTick();

    while (true)
    {
      // No button press for 3 seconds, enter sleep mode
      if (HAL_GetTick() - idleBeginTime > TIMEOUT)
        break;

      if (buttonPressed())
      {
        idleBeginTime = HAL_GetTick();
        if (!startedPressing)
        {
          LED::resetIndex();
          LED::setCurrentPattern(startupPattern);
          startPressTime = HAL_GetTick();
          startedPressing = true;
        }
        else if (HAL_GetTick() - startPressTime > LONG_PRESS_LENGTH)
        {
          // Wait for the button to be released
          while (buttonPressed())
          {
            HAL_Delay(100);
          }
          // Long press detected, start up the power
          enablePower(true);
          idleBeginTime = 0;
          startedPressing = false;
          running = true;
          break;
        }
      }
      else
      {
        auto batteryValuePattern = voltageToPattern(VoltMeter::getVoltage());
        LED::setCurrentPattern(batteryValuePattern);

        startedPressing = false;
      }

      HAL_Delay(100);
    }

    while (running)
    {

      if (buttonPressed())
      {
        // Press once
        if (HAL_GetTick() - idleBeginTime > TIMEOUT)
        {
          idleBeginTime = HAL_GetTick();
          LED::setCurrentPattern(confirmShutdownPattern);
          while (buttonPressed())
            ;
          continue;
        }

        idleBeginTime = HAL_GetTick();
        if (!startedPressing)
        {
          LED::resetIndex();
          LED::setCurrentPattern(shutdownPattern);
          startPressTime = HAL_GetTick();
          startedPressing = true;
        }
        else if (HAL_GetTick() - startPressTime > LONG_PRESS_LENGTH)
        {
          // Wait for the button to be released
          while (buttonPressed())
          {
            HAL_Delay(100);
          }
          // Long press detected, start up the power
          enablePower(false);
          running = false;
          break;
        }
      }
      else
      {
        if (HAL_GetTick() - idleBeginTime > TIMEOUT)
        {
          auto batteryValuePattern = voltageToPattern(VoltMeter::getVoltage());
          LED::setCurrentPattern(batteryValuePattern);
        }

        startedPressing = false;
      }

      HAL_Delay(100);
    }

    LED::deinit();
    VoltMeter::deinit();
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
  }
}
