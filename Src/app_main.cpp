#include "main.h"
#include "LED.h"
#include "VoltMeter.h"
#include <utility>

static const LED::BlinkPattern startupPattern = {
    .oneshot = true,
    .pattern = {
        {0b0000, 100},
        {0b0001, 350},
        {0b0011, 350},
        {0b0111, 350},
        {0b1111, 350},
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
        {0b1111, 350},
        {0b0111, 350},
        {0b0011, 350},
        {0b0001, 350},
        {0b0000, 100},
    },
};

/**
 * @brief Check if the button is pressed.
 *
 * @return true if the button is pressed
 */
inline bool buttonPressed()
{
  return !HAL_GPIO_ReadPin(Button_GPIO_Port, Button_Pin);
}

/**
 * @brief Enable or disable the power to the device.
 *
 * @param enable true to enable power, false to disable.
 */
inline void enablePower(bool enable)
{
  HAL_GPIO_WritePin(PWREN_GPIO_Port, PWREN_Pin, enable ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
 * @brief Get the battery percentage based on the voltage.
 *
 * @param voltage The battery voltage in volts.
 * @return The battery percentage (0-100).
 */
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

/**
 * @brief Build a LED blinking pattern based on the battery voltage.
 *
 * @param voltage The battery voltage in volts.
 * @return The LED blinking pattern.
 */
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

extern "C"
{
  extern I2C_HandleTypeDef hi2c1;
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
    HAL_I2C_EnableListen_IT(&hi2c1); // Enable I2C listen mode

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
        else
        {
          LED::setCurrentPattern(confirmShutdownPattern);
        }

        startedPressing = false;
      }

      HAL_Delay(100);
    }

    HAL_I2C_DisableListen_IT(&hi2c1); // Disable I2C listen mode
    LED::deinit();
    VoltMeter::deinit();
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
  }
}

/**
 * @brief This function is called when the I2C address is matched.
 *
 * @param hi2c The HAL I2C handle.
 * @param TransferDirection The transfer direction (transmit or receive).
 * @param AddrMatchCode The address match code.
 */
void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode)
{

  if (AddrMatchCode != hi2c->Init.OwnAddress1)
  {
    return; // Ignore if the address doesn't match
  }

  if (TransferDirection == I2C_DIRECTION_RECEIVE)
  {
    auto battLevel = getBatteryPercentage(VoltMeter::getVoltage());
    HAL_I2C_Slave_Seq_Transmit_IT(hi2c, &battLevel, sizeof(battLevel), I2C_LAST_FRAME);
  }
}

/**
 * @brief This function is called when the I2C listen mode is complete.
 *
 * @param hi2c The HAL I2C handle.
 */
void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c)
{
  HAL_I2C_EnableListen_IT(hi2c);
}